#include "raft.h"
#include <iostream>
#include <chrono>
#include "server.h"
#include "database.h"

using json = nlohmann::json;

// Initialiser for Raft_Node
Raft_Node::Raft_Node(int id, int server_count, Server *server)
    : commitIndex(-1)
    , lastApplied(-1)
    , log()
{
    this->candidate_id = id;
    this->term = 0;
    this->leader_id = -1; // -1 if no leader or leader, id of leader otherwise
    this->state = FOLLOWER;
    this->time_of_last_message = (long *)malloc(sizeof(long));
    *(this->time_of_last_message) = 0L;
    this->time_of_last_heartbeat = (long *)malloc(sizeof(long));
    *(this->time_of_last_heartbeat) = 0L;
    this->election_timeout = getElectionTimeout();

    this->nextIndex = (int*)calloc(sizeof(int), server_count);
    for (int i = 0; i < server_count; i++)
        this->nextIndex[i] = 0;

    this->matchIndex = (int*)calloc(sizeof(int), server_count);
    for (int i = 0; i < server_count; i++)
        this->matchIndex[i] = 0;

    this->voted_for_id = -1;
    this->vote_count = 0;

    this->server_count = server_count;

    this->server = server;
}

Raft_Node::~Raft_Node()
{
    // These somehow break everything? Please tell me I'm being dumb
    // free(this->time_of_last_message);
    // free(this->time_of_last_heartbeat);
    // free(this->nextIndex);
    // free(this->matchIndex);
}

// This function is run every millisecond, it checks the timer to see if
// an election can begin
void Raft_Node::run()
{
    if (this->state == LEADER && this->checkHeartbeatTimer())
    {
        this->server->send_details("Sending Heartbeats");

        // send heartbeats
        for (int serverID = 0; serverID < server_count; serverID++)
            if (serverID != candidate_id)
                this->server->sendToServer(this->server->getServerSocketAddress(serverID), this->getAppendEntriesMessage(serverID));
    }

    // If election timer expired, request votes from neighbours
    if (this->checkElectionTimer())
    {
        this->setState(CANDIDATE);
        this->voted_for_id = this -> candidate_id;
        this->vote_count = 1; // 1 because always votes for self
        this->server->sendToAllServers(this->getVoteRequestMessage());
    }

    // If the commit index is less than the apply index
    // then apply up to the commit index
    for (int i = lastApplied + 1; i <= commitIndex; i++)
    {
        auto& entry = log[i];
        this->server->getDatabase()->set_value(entry.index, entry.value);
        lastApplied += 1;
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
                for (int serverID = 0; serverID < server_count; serverID++)
                    if (serverID != candidate_id)
                        this->server->sendToServer(this->server->getServerSocketAddress(serverID), this->getAppendEntriesMessage(serverID));
            
                // Reset leader state
                // Spec has +1 for below, but they want log to start at 1 instead of 0
                for (int i = 0; i < server_count; i++)
                    this->nextIndex[i] = log.size();

                for (int i = 0; i < server_count; i++)
                    this->matchIndex[i] = 0;
            }
        }
    }

    // If message is an append entry, check if valid
    if ((deserialised_json["message_type"] == "append_entries") &&
        (deserialised_json["response"] == "false"))
    {
        this->server->send_details("AppendEntry RPC Received");

        int sender_id = deserialised_json["sender_id"].get<int>();

        if (this->getState() == CANDIDATE){
            this->setState(FOLLOWER);
        }

        // Receiver rule 1
        // Reply false if term < currentTerm (§5.1)
        if (deserialised_json["data"]["term"].get<int>() < term)
        {
            this->server->sendToServer(sender_id, this->getAppendEntriesResponseMessage(false));
            return;
        }

        // Receiver rule 2
        // Reply false if log doesn’t contain an entry at prevLogIndex
        // whose term matches prevLogTerm (§5.3)
        int prevLogIndex = deserialised_json["data"]["prevLogIndex"].get<int>();
        int prevLogTerm = deserialised_json["data"]["prevLogTerm"].get<int>();
        // if it doesn't have an entry, or if it has an entry and it doesn't match
        if (prevLogIndex >= 0 && (prevLogIndex > log.size() || log[prevLogIndex].term != prevLogTerm))
        {
            this->server->sendToServer(sender_id, this->getAppendEntriesResponseMessage(false));
            return;
        }

        int sentEntriesSize = deserialised_json["data"]["entries"].size();
        int totalEntries = prevLogIndex + sentEntriesSize;
        int newEntries = totalEntries - (log.size() - 1);
        int newEntriesStart = sentEntriesSize - newEntries;

        // Receiver rule 3
        // If an existing entry conflicts with a new one (same index
        // but different terms), delete the existing entry and all that
        // follow it (§5.3)
        for (int i = 0; i < newEntriesStart - 1; i++)
        {
            int index = prevLogIndex + i; // + 1?
            // Existing index with conflicting term
            if (log[index].term != deserialised_json["data"]["entries"][i]["term"].get<int>())
            {
                for (int j = index; j < log.size(); j++)
                {
                    log.pop_back();
                }
                break;
            }
        }

        // Receiver rule 4
        // Append any new entries not already in the log
        for (int i = newEntriesStart; i < sentEntriesSize; i++)
        {
            LogEntry entry;
            entry.term = deserialised_json["data"]["entries"][i]["term"].get<int>();
            entry.index = deserialised_json["data"]["entries"][i]["index"].get<int>();
            entry.value = deserialised_json["data"]["entries"][i]["value"].get<int>();
            log.push_back(entry);
        }

        // Receiver rule 5
        // If leaderCommit > commitIndex, set commitIndex =
        // min(leaderCommit, index of last new entry)
        int leaderCommit = deserialised_json["data"]["leaderCommit"].get<int>();
        if (leaderCommit > commitIndex)
        {
            // Minimum of the two
            commitIndex = (leaderCommit < log.size()) ? leaderCommit : log.size();
        }

        this->server->sendToServer(sender_id, this->getAppendEntriesResponseMessage(true));
    }

    if ((deserialised_json["message_type"] == "append_entries") &&
        (deserialised_json["response"] == "true"))
    {
        int sender_id = deserialised_json["sender_id"].get<int>();

        // TODO: check for greater term and step down if applicable

        if (!deserialised_json["data"]["success"].get<bool>())
        {
            nextIndex[sender_id]--;
            if (nextIndex[sender_id] < 0)
                nextIndex[sender_id] = 0;

            printf("Got false success from server %d\n", sender_id);
        }
        else
        {
            int prevLogIndex = deserialised_json["data"]["prevLogIndex"].get<int>();
            nextIndex[sender_id] = prevLogIndex + 1;

            // Find the largest index that a majority of servers have replicated
            int majorityIndex = -1;
            for (int index = prevLogIndex; index >= 0; index--)
            {
                // Count starts at 1 because leader already has it
                int count = 1;
                for (int serverID = 0; serverID < server_count; serverID++)
                    if (nextIndex[serverID] > index)
                        count++;
                
                if (count >= server_count / 2.0f)
                {
                    majorityIndex = index;
                    break;
                }
            }

            // Commit changes in order
            if (commitIndex < majorityIndex)
            {
                commitIndex = majorityIndex;
            }
        }
    }

    // If the message is a data update to the leader then
    // add it to the list of not committed data changes
    // TODO: Maybe forward the update to the leader?
    if (deserialised_json["message_type"] == "data_update" &&
        this->state == LEADER)
    {
        LogEntry entry;
        entry.term = this->term;
        entry.index = deserialised_json["data"]["index"].get<int>();
        entry.value = deserialised_json["data"]["value"].get<int>();

        this->log.push_back(entry);
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
std::string Raft_Node::getAppendEntriesMessage(int server)
{
    int prevLogIndex = nextIndex[server] - 1;
    int prevLogTerm = (prevLogIndex < log.size() && prevLogIndex >= 0) ? log[prevLogIndex].term : 0;

    json appendEntriesJson = {
        {"sender_id", this->candidate_id},
        {"message_type", "append_entries"},
        {"response", "false"},
        {"data",
            {
                {"term", this->term},
                {"leaderId", this->candidate_id},
                {"prevLogIndex", prevLogIndex},
                {"prevLogTerm", prevLogTerm},
                {"leaderCommit", commitIndex},
                {"entries", json::array()}
            }
        }
    };

    for (int i = nextIndex[server]; i < log.size(); i++)
    {
        LogEntry entry = log[i];
        appendEntriesJson["data"]["entries"][i - nextIndex[server]] = {
            {"term", entry.term},
            {"index", entry.index},
            {"value", entry.value}
        };
    }

    return appendEntriesJson.dump();
}

// Returns a string representation of a response message to send
std::string Raft_Node::getAppendEntriesResponseMessage(bool success)
{
    int prevLogIndex = log.size() - 1;

    // NOTE(Josh): `prevLogIndex` isn't a part of the formal specification
    // I added it because I can't work out how else we work out what
    // a given server has added to its log
    json append_entries_json = {
        {"sender_id", this->candidate_id},
        {"message_type", "append_entries"},
        {"response", "true"},
        {"data",
            {
                {"term", this->term},
                {"success", success},
                {"prevLogIndex", prevLogIndex}
            }
        }
    };

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