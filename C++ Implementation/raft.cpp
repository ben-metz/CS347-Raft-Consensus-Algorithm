#include "raft.h"
#include <iostream>
#include <chrono>
#include "server.h"

using json = nlohmann::json;

// Initialiser for Raft_Node
Raft_Node::Raft_Node(int id, int server_count, Server *server)
{
    this->candidate_id = id;
    this->term = 0;
    this->leader_id = -1; // -1 if no leader or leader, id of leader otherwise
    this->state = FOLLOWER;
    this->time_of_last_message = (long *)malloc(sizeof(long));
    this->time_of_last_heartbeat = (long *)malloc(sizeof(long));
    this->election_timeout = getElectionTimeout();

    this->voted_for_id = -1;
    this->vote_count = 0;

    this->server_count = server_count;

    this->server = server;
}

// This function is run every millisecond, it checks the timer to see if
// an election can begin
void Raft_Node::run()
{
    if (this->state == LEADER && this->checkHeartbeatTimer())
    {
        this->server->send_details("Sending Heartbeats");

        // send heartbeats
        this->server->sendToAllServers(this->getAppendEntriesMessage(true));
    }

    // If election timer expired, request votes from neighbours
    if (this->checkElectionTimer())
    {
        this->setState(CANDIDATE);
        this->voted_for_id = this -> candidate_id;
        this->vote_count = 1; // 1 because always votes for self
        this->server->sendToAllServers(this->getVoteRequestMessage());
    }
}

// When a server receives a message, it is fed into this function where
// it is deserialised and the appropriate action is taken
void Raft_Node::input_message(char *msg)
{
    // Reset countdown every time message received (needs to be changed to be only when heartbeat received from leader)
    *(this->time_of_last_message) = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    json deserialised_json = json::parse(std::string(msg));

    // If message is a vote request, respond with a vote response
    if ((deserialised_json["message_type"] == "request_vote") &&
        (deserialised_json["response"] == "false"))
    {
        this->server->send_details("Vote Request Received");

        // If candidate, already voted for themselves
        if (this->state == CANDIDATE){
            this->server->sendToServer(deserialised_json["sender_id"].get<int>(), this->getVoteResponseMessage(false));
        }

        // If received term > nodes term, then step down
        if (deserialised_json["data"]["term"].get<int>() > this->term)
        {
            // Update the node term to the term of received message
            this->term = deserialised_json["data"]["term"].get<int>();

            // Stepped down, so not a follower
            this->setState(FOLLOWER);

            // Reset voted for indicator
            this->voted_for_id = -1;
        }

        // If received term < nodes term, reply false (out of date candidate)
        if (deserialised_json["data"]["term"].get<int>() < this->term)
        {
            this->server->sendToServer(deserialised_json["sender_id"].get<int>(), this->getVoteResponseMessage(false));
        }

        // Don't double vote
        if (this->voted_for_id != -1)
        {
            this->server->sendToServer(deserialised_json["sender_id"].get<int>(), this->getVoteResponseMessage(false));
        }

        // If nobody voted for, then vote for message sender
        if (this->voted_for_id == -1)
        {
            // Reset random timeout
            this->election_timeout = getElectionTimeout();

            // Indicate vote for candidate that sent message
            this->voted_for_id = deserialised_json["data"]["candidate_id"].get<int>();

            // Return successful vote message
            this->server->sendToServer(deserialised_json["sender_id"].get<int>(), this->getVoteResponseMessage(true));
        }
    }

    // If message is a vote response, check if valid
    if ((deserialised_json["message_type"] == "request_vote") &&
        (deserialised_json["response"] == "true"))
    {
        this->server->send_details("Vote Response Received");

        // If received term > nodes term, step down
        if (deserialised_json["data"]["term"].get<int>() > this->term)
        {
            // Update the node term to the term of received message
            this->term = deserialised_json["data"]["term"].get<int>();

            // Stepped down, so not a follower
            this->setState(FOLLOWER);

            // Reset voted for indicator
            this->voted_for_id = -1;
        }

        // If vote granted, add to count
        if (deserialised_json["data"]["vote_granted"] == true)
        {
            if (this->vote_count++ > this->server_count / 2)
            {
                this->setState(LEADER);

                // send out heartbeats to all servers
                this->server->sendToAllServers(this->getAppendEntriesMessage(true));
            }
        }
    }

    // If message is an append entry, check if valid
    if ((deserialised_json["message_type"] == "append_entries") &&
        (deserialised_json["response"] == "false") &&
        (deserialised_json["heartbeat"] == true))
    {
        this->server->send_details("Heartbeat Received");

        if (this->getState() == CANDIDATE){
            this->setState(FOLLOWER);
        }

        this->server->sendToServer(deserialised_json["sender_id"].get<int>(), this->getAppendEntriesResponseMessage(true));
    }
}

// This function checks if the random timeout has expired, if so, take appropriate action
bool Raft_Node::checkElectionTimer()
{
    auto millisec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (millisec_since_epoch - *(this->time_of_last_message) > this->election_timeout)
    {
        // Reset time of last interaction if timeout reached
        *(this->time_of_last_message) = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        // Start next term
        this->term++;

        return true;
    }
    else
    {
        return false;
    }
}

// This function checks if the random timeout has expired, if so, take appropriate action
bool Raft_Node::checkHeartbeatTimer()
{
    auto millisec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (millisec_since_epoch - *(this->time_of_last_heartbeat) > HEARTBEAT_TIMEOUT)
    {
        // Reset time of last interaction if timeout reached
        *(this->time_of_last_heartbeat) = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        return true;
    }
    else
    {
        return false;
    }
}

// Returns a random int between 150 and 300
int Raft_Node::getElectionTimeout()
{
    srand(time(0)); // Set seed to current time
    return 150 + rand() % 150;
}

// Returns ID of server/raft instance
int Raft_Node::getID()
{
    return this->candidate_id;
}

// Returns the vote request message
std::string Raft_Node::getVoteRequestMessage(int last_log_index, int last_log_term)
{
    this->server->send_details("Vote Request Sent");

    // Reset random timeout
    this->election_timeout = getElectionTimeout();

    // Vote for itself
    this->vote_count++;

    json vote_request_data = {
        {"sender_id", this->candidate_id},
        {"message_type", "request_vote"},
        {"response", "false"},
        {"data", {
                     {"term", this->term},                 // candidate's term
                     {"candidate_id", this->candidate_id}, // candidate requesting vote
                     {"last_log_index", last_log_index},   // index of candidate's last log entry
                     {"last_log_term", last_log_term}      // term of candidate's last log entry
                 }}};

    return vote_request_data.dump();
}

// Returns a string representation of a response message to send
std::string Raft_Node::getVoteResponseMessage(bool voteGranted)
{
    this->server->send_details("Sent Vote Response");
    json vote_request_data = {
        {"sender_id", this->candidate_id},
        {"message_type", "request_vote"},
        {"response", "true"},
        {"data", {
                     {"term", this->term},         // candidate's term
                     {"vote_granted", voteGranted} // candidate requesting vote
                 }}};

    return vote_request_data.dump();
}

// Returns the vote request message
std::string Raft_Node::getAppendEntriesMessage(bool heartbeat, int last_log_index, int last_log_term)
{
    json append_entries_json = {
        {"sender_id", this->candidate_id},
        {"message_type", "append_entries"},
        {"response", "false"},
        {"heartbeat", heartbeat},
        {"data", {
                     {"term", this->term},               // candidate's term
                     {"leader_id", this->candidate_id},  // candidate requesting vote
                     {"last_log_index", last_log_index}, // index of candidate's last log entry
                     {"last_log_term", last_log_term},   // term of candidate's last log entry
                     {"leader_commit", -1},              // leader commit
                 }}};

    return append_entries_json.dump();
}

// Returns a string representation of a response message to send
std::string Raft_Node::getAppendEntriesResponseMessage(bool success)
{
    json append_entries_json = {
        {"sender_id", this->candidate_id},
        {"message_type", "append_entries"},
        {"response", "true"},
        {"data", {
                     {"term", this->term},     // candidate's term
                     {"vote_granted", success} // candidate requesting vote
                 }}};

    return append_entries_json.dump();
}

// Sets the state of the node based on passed string for convenience
void Raft_Node::setState(int state)
{
    if (state == 0)
    {
        this->server->send_details("State change: LEADER");
    }
    else if (state == 1)
    {
        this->server->send_details("State change: CANDIDATE");
    }
    else
    {
        this->server->send_details("State change: FOLLOWER");
    }

    this->state = state;
}

// Returns the current state
int Raft_Node::getState()
{
    return this->state;
}

// Returns the current term
int Raft_Node::getTerm()
{
    return this->term;
}

// Returns the current node voted for
int Raft_Node::getVote()
{
    return this->voted_for_id;
}