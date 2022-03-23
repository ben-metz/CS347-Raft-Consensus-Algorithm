#pragma once

#include <string>
#include <vector>
#include "json.hpp"

#define LEADER 0
#define CANDIDATE 1
#define FOLLOWER 2

#define HEARTBEAT_TIMEOUT 100

using json = nlohmann::json;

class Server;

class RaftNode
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

    int electionTimeout;
    int heartbeatTimeout;

    long timeOfLastMessage;
    long timeOfLastHeartbeat;

    int serverCount;

    int term;
    int candidateId;
    int leaderId;

    // Leader election stuff
    int votedForId;
    int voteCount;

    Server *server; // Associated server

    void handleRequestVote(json deserialisedJson);
    void handleGrantRequestVote(int senderId, bool voteGranted);
    void handleDenyRequestVote(int senderId, int candidateId, int lastLogIndex);

    void handleDataUpdate(json deserialisedJson, char* msg);

    void handleAppendEntries(json deserialisedJson);
    void handleAppendEntriesRequest(
        int senderId,
        int leaderId,
        int term,
        int prevLogIndex,
        int prevLogTerm,
        int leaderCommit,
        std::vector<json> entries
    );
    void handleAppendEntriesResponse(int senderId, bool success, int prevLogIndex);

public:
    RaftNode(int id, int serverCount, Server *server);
    ~RaftNode();

    void run();
    void inputMessage(char *msg);

    bool checkElectionTimer();
    bool checkHeartbeatTimer();

    int getElectionTimeout();

    std::string getVoteRequestMessage(int lastLogIndex, int lastLogTerm);
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