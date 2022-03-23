#pragma once

#include <string>
#include <vector>

#define LEADER 0
#define CANDIDATE 1
#define FOLLOWER 2

#define HEARTBEAT_TIMEOUT 100

class Server;

class Raft_Node
{
private:
    int state;

    struct LogEntry
    {
        int term;
        int index;
        int value;
    };
    std::vector<LogEntry> log;

    // Index of highest log entry known to be committed
    int commitIndex;
    // Index of highest log entry applied to state machine
    int lastApplied;

    // For each server, index of the next
    // log entry to send to that server
    int* nextIndex;
    // For each server, index of the highest log
    // entry known to be replicated on that server
    int* matchIndex;

    int election_timeout;
    int heartbeat_timeout;

    long time_of_last_message;
    long time_of_last_heartbeat;

    int server_count;

    int term;
    int candidate_id;
    int leader_id;

    // Leader election stuff
    int voted_for_id;
    int vote_count;

    Server *server; // Associated server

    void handleRequestVote(json deserialised_json);
    void handleGrantRequestVote(int sender_id, bool vote_granted);
    void handleDenyRequestVote(int sender_id, int candidate_id, int last_log_index);

public:
    Raft_Node(int id, int server_count, Server *server);
    ~Raft_Node();

    void run();
    void input_message(char *msg);

    bool checkElectionTimer();
    bool checkHeartbeatTimer();

    int getElectionTimeout();

    std::string getVoteRequestMessage(int last_log_index = -1, int last_log_term = -1);
    std::string getVoteResponseMessage(bool voteGranted);

    std::string getAppendEntriesMessage(int server);
    std::string getAppendEntriesResponseMessage(bool success);

    int getID();
    int getState();
    int getTerm();
    int getVote();

    void setState(int state);

    int getCommitIndex();

    void resetElectionTimer();
    void setTimeout(int time);

    void setTerm(int term);
};