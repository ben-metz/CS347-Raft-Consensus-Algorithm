#include "raft.h"
#include <iostream>
#include <chrono>
#include "server.h"

using json = nlohmann::json;

Raft_Node::Raft_Node(int id, int server_count, Server *server)
{
    this->candidate_id = id;
    this->term = 0;
    this->leader_id = -1; // -1 if no leader or leader, id of leader otherwise
    this->state = 0;
    this->time_of_last_message = (long *)malloc(sizeof(long));
    this->random_timeout = getRandomTimeout();

    this->voted_for_id = id;
    this->vote_count = 0;

    this->server_count = server_count;

    this->server = server;
}

void Raft_Node::run(){
    if(this->checkTimer()){
        std::cout << "REQUESTING VOTES";
        this -> setState("Candidate");
        this->server->sendToAllServers(this->getVoteRequestMessage());
    }
}

void Raft_Node::input_message(char *msg, char *output_buffer)
{
    // ATM only displays raft input
    std::cout << "Raft Input: " << msg << '\n';

    // Reset countdown every time message received (needs to be changed to be only when heartbeat received from leader)
    *(this->time_of_last_message) = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    json deserialised_json = json::parse(std::string(msg));

    // If message is a vote request, respond with a vote response
    if ((deserialised_json["message_type"] == "request_vote") &&
        (deserialised_json["response"] == "false"))
    {
        // If received term < nodes term, reply false
        if (deserialised_json["data"]["term"].get<int>() < this->term)
        {
            std::string resp = this->getVoteResponseMessage(false);
            memcpy(output_buffer, resp.c_str(), resp.size());
        }

        // If nobody voted for, then vote for message sender
        if (this->voted_for_id == this->candidate_id)
        {
            // Reset random timeout
            this->random_timeout = getRandomTimeout();

            // Get response message
            std::string resp = this->getVoteResponseMessage(true);
            memcpy(output_buffer, resp.c_str(), resp.size());

            this->voted_for_id = deserialised_json["data"]["candidate_id"].get<int>();

            this->setState("Follower");
        }
    }

    // If message is a vote response, check if valid
    if ((deserialised_json["message_type"] == "request_vote") &&
        (deserialised_json["response"] == "true"))
    {
        // If received term < nodes term, reply false
        if (deserialised_json["data"]["vote_granted"] == "true")
        {
            this->vote_count++;

            if (this->vote_count > this->server_count / 2)
            {
                this->setState("Leader");

                // send out appendentries to all servers
                // this -> server -> sendToAllServers(this->getAppendEntriesMessage());
            }
        }
    }

    std::cout << "Raft Response: " << output_buffer << "\n\n";
}

bool Raft_Node::checkTimer()
{
    auto millisec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (millisec_since_epoch - *(this->time_of_last_message) > this->random_timeout)
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

// Returns a random int between 150 and 300
int Raft_Node::getRandomTimeout()
{
    srand(time(0));
    return 150 + rand() % 150;
}

int Raft_Node::getID()
{
    return this->candidate_id;
}

// Returns the vote request message
std::string Raft_Node::getVoteRequestMessage(int last_log_index, int last_log_term)
{
    // Reset random timeout
    this->random_timeout = getRandomTimeout();

    // Vote for itself
    this->vote_count++;

    json vote_request_data = {
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

std::string Raft_Node::getVoteResponseMessage(bool voteGranted)
{
    json vote_request_data = {
        {"message_type", "request_vote"},
        {"response", "true"},
        {"data", {
                     {"term", this->term},         // candidate's term
                     {"vote_granted", voteGranted} // candidate requesting vote
                 }}};

    return vote_request_data.dump();
}

void Raft_Node::setState(std::string state)
{
    if (state == "Leader")
    {
        this->state = 0;
    }

    if (state == "Candidate")
    {
        this->state = 1;
    }

    if (state == "Follower")
    {
        this->state = 2;
    }
}

int Raft_Node::getState(){
    return this->state;
}

int Raft_Node::getTerm(){
    return this->term;
}

int Raft_Node::getVote(){
    return this->voted_for_id;
}