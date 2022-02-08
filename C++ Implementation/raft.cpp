#include "raft.h"
#include <iostream>
#include <chrono>
#include "json.hpp"

Raft_Node::Raft_Node(int id){
    this -> candidate_id = id;
    this -> term = 0;
    this -> leader_id = -1; // -1 if no leader or leader, id of leader otherwise
    this -> state = 0;
    this -> time_of_last_message = (long*) malloc(sizeof(long));
    this -> random_timeout = getRandomTimeout();
}

void Raft_Node::input_message(char* msg, char* output_buffer){
    // Reset countdown every time message received (needs to be changed to be only when heartbeat received from leader)
    *(this -> time_of_last_message) = (long) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    // ATM only displays raft input
    std::cout << "Raft Input: " << msg << '\n';

    int i = 0;
    while(msg[i] != '\0'){
        output_buffer[i] = msg[i];
        i+=1;
    }
    output_buffer[i] = '\0';

    std::cout << "Raft Response: " << output_buffer << "\n\n";
}

bool Raft_Node::checkTimer(){
    auto millisec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (millisec_since_epoch - *(this -> time_of_last_message) > this -> random_timeout){
        // Reset time of last interaction if timeout reached
        *(this -> time_of_last_message) = (long) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        
        return true;
    } else {
        return false;
    }
}

// Returns a random int between 150 and 300
int Raft_Node::getRandomTimeout(){
    srand(time(0));
    return 150 + rand()%150;
}

int Raft_Node::getID() {
    return this -> candidate_id;
}

using json = nlohmann::json;

// Returns the vote request message
std::string Raft_Node::getVoteRequestMessage(int last_log_index, int last_log_term){
    // Reset random timeout
    this -> random_timeout = getRandomTimeout();

    json vote_request_data = {
        {"message_type", "request_vote"},
        {"data", {
            {"term", this -> term}, // candidate's term
            {"candidate_id", this -> candidate_id}, // candidate requesting vote
            {"last_log_index", last_log_index}, // index of candidate's last log entry
            {"last_log_term", last_log_term} // term of candidate's last log entry
        }}
    };

    return vote_request_data.dump();
}