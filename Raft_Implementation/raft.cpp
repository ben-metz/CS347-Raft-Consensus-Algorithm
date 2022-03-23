#include "raft.h"
#include <iostream>
#include <chrono>
#include "server.h"
#include "database.h"
#include <string>

#define DATA_UPDATE_MESSAGE "data_update"
#define REQUEST_VOTE_MESSAGE "request_vote"
#define APPEND_ENTRIES_MESSAGE "append_entries"

using json = nlohmann::json;

// Initialiser for RaftNode
RaftNode::RaftNode(int id, int serverCount, Server *server)
    : candidateId(id)
    , term(0)
    , leaderId(-1) // -1 if no leader or leader, id of leader otherwise
    , state(FOLLOWER)
    , timeOfLastMessage(0L)
    , timeOfLastHeartbeat(0L)
    , votedForId(-1)
    , voteCount(0)
    , serverCount(serverCount)
    , server(server)
    , commitIndex(-1)
    , lastApplied(-1)
    , log()
    // Set in constructor body
    , nextIndex(nullptr)
    , matchIndex(nullptr)
{
    this->resetElectionTimer();

    this->nextIndex = (int*)calloc(sizeof(int), serverCount);
    for (int i = 0; i < serverCount; i++)
        this->nextIndex[i] = 0;

    this->matchIndex = (int*)calloc(sizeof(int), serverCount);
    for (int i = 0; i < serverCount; i++)
        this->matchIndex[i] = 0;
}

RaftNode::~RaftNode()
{
    free(this->nextIndex);
    this->nextIndex = nullptr;
    
    free(this->matchIndex);
    this->matchIndex = nullptr;
}

// This function is run every millisecond, it checks the timer to see if
// an election can begin
void RaftNode::run()
{
    // If this node is the leader and the heartbeat timer has expired
    if (this->state == LEADER && this->checkHeartbeatTimer())
    {
        this->server->sendDetails("Sending Heartbeats");

        // Send heartbeats to all servers
        // NOTE: Can't broadcast as each server has a unique AppendEntriesRPC
        for (int serverID = 0; serverID < serverCount; serverID++)
            if (serverID != candidateId)
                this->server->sendToServer(this->server->getServerSocketAddress(serverID), this->getAppendEntriesMessage(serverID));
    }

    // If election timer expired, request votes from neighbours
    if (this->checkElectionTimer())
    {
        this->setState(CANDIDATE);

        this->term++;

        this->votedForId = this -> candidateId;
        this->voteCount = 1; // 1 because always votes for self

        // Reset random timeout
        this->resetElectionTimer();
        
        this->server->sendToAllServers(this->getVoteRequestMessage(this->commitIndex, -1));
    }

    // If the commit index is less than the apply index
    // then apply up to the commit index
    for (int i = lastApplied + 1; i <= commitIndex; i++)
    {
        auto& entry = log[i];
        this->server->getDatabase()->setValue(entry.index, entry.value);
        lastApplied += 1;
    }
}

void RaftNode::resetElectionTimer(){
    // Reset election timeout
    this->electionTimeout = getElectionTimeout();

    // Reset time of last interaction if timeout reached
    this->timeOfLastMessage = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void RaftNode::setTimeout(int time)
{
    // Set election timer
    if (this->state != LEADER){
        this->electionTimeout = time;
    } else {
        this->heartbeatTimeout = time;
    }

    // Reset time of last interaction if timeout reached
    this->timeOfLastMessage = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void RaftNode::setTerm(int term)
{
    // Set election timer
    this->term = term;
}

void RaftNode::handleDenyRequestVote(int senderId, int candidateId, int lastLogIndex)
{
    std::ostringstream s;

    // §5.2, §5.4: If votedFor is null or candidateId, and candidate’s log is at least as up-to-date as receiver’s log, grant vote
    if (this->votedForId == -1 && lastLogIndex >= this->commitIndex)
    {
        
        s << "Vote Request from " << senderId << " granted";
        std::string resp(s.str());

        this->server->sendDetails(resp);

        // Indicate vote for candidate that sent message
        this->votedForId = candidateId;

        // Return successful vote message
        this->server->sendToServer(senderId, this->getVoteResponseMessage(true));

        // Reset election timeout
        this->resetElectionTimer();

        return;
    }

    s << "Vote Request from " << senderId << " denied";
    std::string resp(s.str());

    this->server->sendToServer(senderId, this->getVoteResponseMessage(false));

    this->server->sendDetails(resp);
}

void RaftNode::handleGrantRequestVote(int senderId, bool voteGranted)
{
    std::ostringstream s;
    s << "Vote Response from " << senderId << ": " << std::boolalpha << voteGranted;
    std::string resp(s.str());

    this->server->sendDetails(resp);

    // If a vote is granted, then add it to the count
    if (voteGranted && this->state == CANDIDATE)
    {
        // If we get a majority vote then become the leader
        if (++this->voteCount > this->serverCount / 2.0f)
        {
            this->setState(LEADER);

            // Send out heartbeats to all servers
            for (int serverID = 0; serverID < serverCount; serverID++)
                if (serverID != candidateId)
                    this->server->sendToServer(this->server->getServerSocketAddress(serverID), this->getAppendEntriesMessage(serverID));
        
            // Reset leader state
            // Specification has +1 for below, but they want log to start at 1 instead of 0
            for (int i = 0; i < serverCount; i++)
                this->nextIndex[i] = log.size();

            for (int i = 0; i < serverCount; i++)
                this->matchIndex[i] = 0;
        }
    }
}

void RaftNode::handleRequestVote(json deserialisedJson)
{
    bool response = deserialisedJson["response"].get<bool>();

    if (response) {
        this->handleGrantRequestVote(deserialisedJson["sender_id"].get<int>(), deserialisedJson["data"]["vote_granted"].get<bool>());
    } else {
        this->handleDenyRequestVote(deserialisedJson["sender_id"].get<int>(), deserialisedJson["data"]["candidateId"].get<int>(), deserialisedJson["data"]["last_log_index"].get<int>());
    }
}

void RaftNode::handleDataUpdate(json deserialisedJson, char* msg)
{
    // Apply change to the log if leader
    if (this->state == LEADER)
    {
        LogEntry entry;
        entry.term = this->term;
        entry.index = deserialisedJson["data"]["index"].get<int>();
        entry.value = deserialisedJson["data"]["value"].get<int>();

        this->log.push_back(entry);
    }
    // Otherwise forward it to the leader (if one exists)
    else if (leaderId >= 0)
    {
        this->server->sendToServer(this->server->getServerSocketAddress(leaderId), msg);
    }
}

void RaftNode::handleAppendEntriesResponse(int senderId, bool success, int prevLogIndex)
{
    // If the AppendEntriesRPC failed then decrement
    // the next log entry to send to that server
    if (!success)
    {
        nextIndex[senderId]--;
        if (nextIndex[senderId] < 0)
            nextIndex[senderId] = 0;
    }
    else
    {
        // If it succeeded, then update the next index to be the previous log index (+1)
        nextIndex[senderId] = prevLogIndex + 1;

        // Find the largest index that a majority of servers have replicated
        int majorityIndex = -1;
        for (int index = prevLogIndex; index >= 0; index--)
        {
            // Count starts at 1 because leader (this node) already has that entry
            int count = 1;
            for (int serverID = 0; serverID < serverCount; serverID++)
                if (nextIndex[serverID] > index)
                    count++;
            
            if (count >= serverCount / 2.0f)
            {
                majorityIndex = index;
                break;
            }
        }

        // If a majority of servers have replicated a given log
        // entry then we can commit up to that majority index
        if (commitIndex < majorityIndex)
        {
            commitIndex = majorityIndex;
        }
    }
}

void RaftNode::handleAppendEntriesRequest(
    int senderId,
    int leaderId,
    int term,
    int prevLogIndex,
    int prevLogTerm,
    int leaderCommit,
    std::vector<json> entries
)
{

    std::ostringstream s;
    s << "AppendEntry RPC from " << senderId;
    std::string resp(s.str());
    
    this->server->sendDetails(resp);

    // Reset random timeout
    this->resetElectionTimer();

    this->voteCount = 0;

    if (this->getState() == CANDIDATE)
    {
        this->setState(FOLLOWER);
    }

    // Update the leader ID to allow for data forwarding
    this->leaderId = leaderId;

    // Receiver rule 1
    // Reply false if term < currentTerm (§5.1)
    if (term < this->term)
    {
        this->server->sendToServer(senderId, this->getAppendEntriesResponseMessage(false));
        return;
    }

    // Receiver rule 2
    // Reply false if log doesn’t contain an entry at prevLogIndex
    // whose term matches prevLogTerm (§5.3)
    // if it doesn't have an entry, or if it has an entry and it doesn't match
    if (prevLogIndex >= 0 && (prevLogIndex > log.size() || log[prevLogIndex].term != prevLogTerm))
    {
        this->server->sendToServer(senderId, this->getAppendEntriesResponseMessage(false));
        return;
    }

    int sentEntriesSize = entries.size();
    int totalEntries = prevLogIndex + sentEntriesSize;
    int newEntries = totalEntries - (log.size() - 1);
    int newEntriesStart = sentEntriesSize - newEntries;

    // Receiver rule 3
    // If an existing entry conflicts with a new one (same index
    // but different terms), delete the existing entry and all that
    // follow it (§5.3)
    for (int i = 0; i < newEntriesStart - 1; i++)
    {
        int index = prevLogIndex + i;
        // Existing index with conflicting term
        if (log[index].term != entries[i]["term"].get<int>())
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
        entry.term = entries[i]["term"].get<int>();
        entry.index = entries[i]["index"].get<int>();
        entry.value = entries[i]["value"].get<int>();
        log.push_back(entry);
    }

    // Receiver rule 5
    // If leaderCommit > commitIndex, set commitIndex =
    // min(leaderCommit, index of last new entry)
    if (leaderCommit > this->commitIndex)
    {
        this->commitIndex = (leaderCommit < log.size()) ? leaderCommit : log.size();
    }

    this->server->sendToServer(senderId, this->getAppendEntriesResponseMessage(true));
}

void RaftNode::handleAppendEntries(json deserialisedJson)
{
    if (deserialisedJson["response"])
    {
        this->handleAppendEntriesResponse(
            deserialisedJson["sender_id"].get<int>(),
            deserialisedJson["data"]["success"].get<bool>(),
            deserialisedJson["data"]["prevLogIndex"].get<int>()
        );
    }
    else
    {
        this->handleAppendEntriesRequest(
            deserialisedJson["sender_id"].get<int>(),
            deserialisedJson["data"]["leaderId"].get<int>(),
            deserialisedJson["data"]["term"].get<int>(),
            deserialisedJson["data"]["prevLogIndex"].get<int>(),
            deserialisedJson["data"]["prevLogTerm"].get<int>(),
            deserialisedJson["data"]["leaderCommit"].get<int>(),
            deserialisedJson["data"]["entries"]
        );
    }
}

// When a server receives a message, it is fed into this function where
// it is deserialised and the appropriate action is taken
void RaftNode::inputMessage(char *msg)
{
    // Reset countdown every time message received (needs to be changed to be only when heartbeat received from leader)
    this->timeOfLastMessage = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    json deserialisedJson = json::parse(std::string(msg));

    // If received term > nodes term, then step down
    if ((deserialisedJson["message_type"] != DATA_UPDATE_MESSAGE) && (deserialisedJson["data"]["term"].get<int>() > this->term))
    {
        // Update the node term to the term of received message
        this->term = deserialisedJson["data"]["term"].get<int>();

        // Stepped down, so not a follower
        this->setState(FOLLOWER);

        // Reset voted for indicator
        this->votedForId = -1;

        this -> voteCount = 0;
    }

    // If message is a vote request, respond with a vote response
    if (deserialisedJson["message_type"] == REQUEST_VOTE_MESSAGE)
    {
        this->handleRequestVote(deserialisedJson);
    }

    if (deserialisedJson["message_type"] == APPEND_ENTRIES_MESSAGE)
    {
        this->handleAppendEntries(deserialisedJson);
    }

    // Handle a data update message
    if (deserialisedJson["message_type"] == DATA_UPDATE_MESSAGE)
    {
        this->handleDataUpdate(deserialisedJson, msg);
    }
}

// This function checks if the random timeout has expired, if so, take appropriate action
bool RaftNode::checkElectionTimer()
{
    auto millisecSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    bool isTimeoutReached = millisecSinceEpoch - this->timeOfLastMessage > this->electionTimeout;

    if (isTimeoutReached)
    {
        // Reset time of last interaction if timeout reached
        this->timeOfLastMessage = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    return isTimeoutReached;
}

// This function checks if the random timeout has expired, if so, take appropriate action
bool RaftNode::checkHeartbeatTimer()
{
    auto millisecSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    bool isTimeoutReached = millisecSinceEpoch - this->timeOfLastHeartbeat > HEARTBEAT_TIMEOUT;

    if (isTimeoutReached)
    {
        // Reset time of last interaction if timeout reached
        this->timeOfLastHeartbeat = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    return isTimeoutReached;
}

// Returns a random int between 150 and 300
int RaftNode::getElectionTimeout()
{
    srand(time(0)); // Set seed to current time
    return 150 + rand() % 150;
}

// Returns ID of server/raft instance
int RaftNode::getID()
{
    return this->candidateId;
}

// Returns the vote request message
std::string RaftNode::getVoteRequestMessage(int lastLogIndex, int lastLogTerm)
{
    //this->server->sendDetails("Vote Request Sent");

    json voteRequestData = {
        {"sender_id", this->candidateId},
        {"message_type", REQUEST_VOTE_MESSAGE},
        {"response", false},
        {"data", {
                     {"term", this->term},                 // candidate's term
                     {"candidateId", this->candidateId}, // candidate requesting vote
                     {"last_log_index", lastLogIndex},   // index of candidate's last log entry
                     {"last_log_term", lastLogTerm}      // term of candidate's last log entry
                 }}};

    return voteRequestData.dump();
}

// Returns a string representation of a response message to send
std::string RaftNode::getVoteResponseMessage(bool voteGranted)
{
    //this->server->sendDetails("Sent Vote Response");

    json voteRequestData = {
        {"sender_id", this->candidateId},
        {"message_type", REQUEST_VOTE_MESSAGE},
        {"response", true},
        {"data", {
                     {"term", this->term},         // candidate's term
                     {"vote_granted", voteGranted} // candidate requesting vote
                 }}};

    return voteRequestData.dump();
}

// Returns the vote request message
std::string RaftNode::getAppendEntriesMessage(int server)
{
    int prevLogIndex = nextIndex[server] - 1;
    int prevLogTerm = (prevLogIndex < log.size() && prevLogIndex >= 0) ? log[prevLogIndex].term : 0;

    //this->server->sendDetails("AppendEntries Sent");

    json appendEntriesJson = {
        {"sender_id", this->candidateId},
        {"message_type", APPEND_ENTRIES_MESSAGE},
        {"response", false},
        {"data",
            {
                {"term", this->term},
                {"leaderId", this->candidateId},
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
std::string RaftNode::getAppendEntriesResponseMessage(bool success)
{
    int prevLogIndex = log.size() - 1;

    // NOTE: `prevLogIndex` isn't a part of the formal specification
    // it was added because can't work out how else we find out what
    // a given server has added to its log
    json appendEntriesJson = {
        {"sender_id", this->candidateId},
        {"message_type", APPEND_ENTRIES_MESSAGE},
        {"response", true},
        {"data",
            {
                {"term", this->term},
                {"success", success},
                {"prevLogIndex", prevLogIndex}
            }
        }
    };

    return appendEntriesJson.dump();
}

// Sets the state of the node based on passed string for convenience
void RaftNode::setState(int state)
{
    if (state == 0)
    {
        this->server->sendDetails("State change: Leader");
    }
    else if (state == 1)
    {
        this->server->sendDetails("State change: Candidate");
    }
    else
    {
        this->server->sendDetails("State change: Follower");
    }

    this->state = state;
}

// Returns the current state
int RaftNode::getState()
{
    return this->state;
}

// Returns the current term
int RaftNode::getTerm()
{
    return this->term;
}

// Returns the current node voted for
int RaftNode::getVote()
{
    return this->votedForId;
}

int RaftNode::getCommitIndex(){
    return this->commitIndex;
}