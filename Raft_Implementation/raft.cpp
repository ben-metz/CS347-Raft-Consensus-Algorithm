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

// Initialiser for Raft_Node
Raft_Node::Raft_Node(int id, int server_count, Server *server)
    : candidate_id(id)
    , term(0)
    , leader_id(-1) // -1 if no leader or leader, id of leader otherwise
    , state(FOLLOWER)
    , time_of_last_message(0L)
    , time_of_last_heartbeat(0L)
    , voted_for_id(-1)
    , vote_count(0)
    , server_count(server_count)
    , server(server)
    , commitIndex(-1)
    , lastApplied(-1)
    , log()
    // Set in constructor body
    , nextIndex(nullptr)
    , matchIndex(nullptr)
{
    this->resetElectionTimer();

    this->nextIndex = (int*)calloc(sizeof(int), server_count);
    for (int i = 0; i < server_count; i++)
        this->nextIndex[i] = 0;

    this->matchIndex = (int*)calloc(sizeof(int), server_count);
    for (int i = 0; i < server_count; i++)
        this->matchIndex[i] = 0;
}

Raft_Node::~Raft_Node()
{
    free(this->nextIndex);
    this->nextIndex = nullptr;
    
    free(this->matchIndex);
    this->matchIndex = nullptr;
}

// This function is run every millisecond, it checks the timer to see if
// an election can begin
void Raft_Node::run()
{
    if (this->state == LEADER && this->checkHeartbeatTimer())
    {
        this->server->sendDetails("Sending Heartbeats");

        // send heartbeats
        for (int serverID = 0; serverID < server_count; serverID++)
            if (serverID != candidate_id)
                this->server->sendToServer(this->server->getServerSocketAddress(serverID), this->getAppendEntriesMessage(serverID));
    }

    // If election timer expired, request votes from neighbours
    if (this->checkElectionTimer())
    {
        this->setState(CANDIDATE);

        this -> term++;

        this->voted_for_id = this -> candidate_id;
        this->vote_count = 1; // 1 because always votes for self

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

void Raft_Node::resetElectionTimer(){
    // Reset election timeout
    this->election_timeout = getElectionTimeout();

    // Reset time of last interaction if timeout reached
    this->time_of_last_message = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void Raft_Node::setTimeout(int time)
{
    // Set election timer
    if (this->state != LEADER){
        this->election_timeout = time;
    } else {
        this->heartbeat_timeout = time;
    }

    // Reset time of last interaction if timeout reached
    this->time_of_last_message = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void Raft_Node::setTerm(int term)
{
    // Set election timer
    this->term = term;
}

void Raft_Node::handleDenyRequestVote(int sender_id, int candidate_id, int last_log_index)
{
    std::ostringstream s;

    // §5.2, §5.4: If votedFor is null or candidateId, and candidate’s log is at least as up-to-date as receiver’s log, grant vote
    if (this->voted_for_id == -1 && last_log_index >= this->commitIndex)
    {
        
        s << "Vote Request from " << sender_id << " granted";
        std::string resp(s.str());

        this->server->sendDetails(resp);

        // Indicate vote for candidate that sent message
        this->voted_for_id = candidate_id;

        // Return successful vote message
        this->server->sendToServer(sender_id, this->getVoteResponseMessage(true));

        // Reset election timeout
        this->resetElectionTimer();

        return;
    }

    s << "Vote Request from " << sender_id << " denied";
    std::string resp(s.str());

    this->server->sendToServer(sender_id, this->getVoteResponseMessage(false));

    this->server->sendDetails(resp);
}

void Raft_Node::handleGrantRequestVote(int sender_id, bool vote_granted)
{
    std::ostringstream s;
    s << "Vote Response from " << sender_id << ": " << std::boolalpha << vote_granted;
    std::string resp(s.str());

    this->server->sendDetails(resp);

    // If vote granted, add to count
    if (vote_granted && this -> state == CANDIDATE)
    {            
        if (++this->vote_count > this->server_count / 2.0f)
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

void Raft_Node::handleRequestVote(json deserialised_json)
{
    bool response = deserialised_json["response"].get<bool>();

    if (response) {
        this->handleGrantRequestVote(deserialised_json["sender_id"].get<int>(), deserialised_json["data"]["vote_granted"].get<bool>());
    } else {
        this->handleDenyRequestVote(deserialised_json["sender_id"].get<int>(), deserialised_json["data"]["candidate_id"].get<int>(), deserialised_json["data"]["last_log_index"].get<int>());
    }
}

void Raft_Node::handleDataUpdate(json deserialised_json, char* msg)
{
    // Apply change to the log if leader
    if (this->state == LEADER)
    {
        LogEntry entry;
        entry.term = this->term;
        entry.index = deserialised_json["data"]["index"].get<int>();
        entry.value = deserialised_json["data"]["value"].get<int>();

        this->log.push_back(entry);
    }
    // Otherwise forward it to the leader (if one exists)
    else if (leader_id >= 0)
    {
        this->server->sendToServer(this->server->getServerSocketAddress(leader_id), msg);
    }
}

void Raft_Node::handleAppendEntriesResponse(int sender_id, bool success, int prevLogIndex)
{
    if (!success)
    {
        nextIndex[sender_id]--;
        if (nextIndex[sender_id] < 0)
            nextIndex[sender_id] = 0;
    }
    else
    {
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

void Raft_Node::handleAppendEntriesRequest(
    int sender_id,
    int leader_id,
    int term,
    int prevLogIndex,
    int prevLogTerm,
    int leaderCommit,
    std::vector<json> entries
)
{

    std::ostringstream s;
    s << "AppendEntry RPC from " << sender_id;
    std::string resp(s.str());
    
    this->server->sendDetails(resp);

    // Reset random timeout
    this->resetElectionTimer();

    this -> vote_count = 0;

    if (this->getState() == CANDIDATE){
        this->setState(FOLLOWER);
    }

    // Update the leader ID to allow for data forwarding
    this->leader_id = leader_id;

    // Receiver rule 1
    // Reply false if term < currentTerm (§5.1)
    if (term < this->term)
    {
        this->server->sendToServer(sender_id, this->getAppendEntriesResponseMessage(false));
        return;
    }

    // Receiver rule 2
    // Reply false if log doesn’t contain an entry at prevLogIndex
    // whose term matches prevLogTerm (§5.3)
    // if it doesn't have an entry, or if it has an entry and it doesn't match
    if (prevLogIndex >= 0 && (prevLogIndex > log.size() || log[prevLogIndex].term != prevLogTerm))
    {
        this->server->sendToServer(sender_id, this->getAppendEntriesResponseMessage(false));
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
        int index = prevLogIndex + i; // + 1?
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
        // Minimum of the two
        this->commitIndex = (leaderCommit < log.size()) ? leaderCommit : log.size();
    }

    this->server->sendToServer(sender_id, this->getAppendEntriesResponseMessage(true));
}

void Raft_Node::handleAppendEntries(json deserialised_json)
{
    if (deserialised_json["response"])
    {
        this->handleAppendEntriesResponse(
            deserialised_json["sender_id"].get<int>(),
            deserialised_json["data"]["success"].get<bool>(),
            deserialised_json["data"]["prevLogIndex"].get<int>()
        );
    }
    else
    {
        this->handleAppendEntriesRequest(
            deserialised_json["sender_id"].get<int>(),
            deserialised_json["data"]["leaderId"].get<int>(),
            deserialised_json["data"]["term"].get<int>(),
            deserialised_json["data"]["prevLogIndex"].get<int>(),
            deserialised_json["data"]["prevLogTerm"].get<int>(),
            deserialised_json["data"]["leaderCommit"].get<int>(),
            deserialised_json["data"]["entries"]
        );
    }
}

// When a server receives a message, it is fed into this function where
// it is deserialised and the appropriate action is taken
void Raft_Node::inputMessage(char *msg)
{
    // Reset countdown every time message received (needs to be changed to be only when heartbeat received from leader)
    this->time_of_last_message = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    json deserialised_json = json::parse(std::string(msg));

    // If received term > nodes term, then step down
    if ((deserialised_json["message_type"] != DATA_UPDATE_MESSAGE) && (deserialised_json["data"]["term"].get<int>() > this->term))
    {
        // Update the node term to the term of received message
        this->term = deserialised_json["data"]["term"].get<int>();

        // Stepped down, so not a follower
        this->setState(FOLLOWER);

        // Reset voted for indicator
        this->voted_for_id = -1;

        this -> vote_count = 0;
    }

    // If message is a vote request, respond with a vote response
    if (deserialised_json["message_type"] == REQUEST_VOTE_MESSAGE)
    {
        this->handleRequestVote(deserialised_json);
    }

    if (deserialised_json["message_type"] == APPEND_ENTRIES_MESSAGE)
    {
        this->handleAppendEntries(deserialised_json);
    }

    // Handle a data update message
    if (deserialised_json["message_type"] == DATA_UPDATE_MESSAGE)
    {
        this->handleDataUpdate(deserialised_json, msg);
    }
}

// This function checks if the random timeout has expired, if so, take appropriate action
bool Raft_Node::checkElectionTimer()
{
    auto millisec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    bool is_timeout_reached = millisec_since_epoch - this->time_of_last_message > this->election_timeout;

    if (is_timeout_reached)
    {
        // Reset time of last interaction if timeout reached
        this->time_of_last_message = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    return is_timeout_reached;
}

// This function checks if the random timeout has expired, if so, take appropriate action
bool Raft_Node::checkHeartbeatTimer()
{
    auto millisec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    bool is_timeout_reached = millisec_since_epoch - this->time_of_last_heartbeat > HEARTBEAT_TIMEOUT;

    if (is_timeout_reached)
    {
        // Reset time of last interaction if timeout reached
        this->time_of_last_heartbeat = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    return is_timeout_reached;
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
    //this->server->sendDetails("Vote Request Sent");

    json vote_request_data = {
        {"sender_id", this->candidate_id},
        {"message_type", REQUEST_VOTE_MESSAGE},
        {"response", false},
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
    //this->server->sendDetails("Sent Vote Response");

    json vote_request_data = {
        {"sender_id", this->candidate_id},
        {"message_type", REQUEST_VOTE_MESSAGE},
        {"response", true},
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

    //this->server->sendDetails("AppendEntries Sent");

    json appendEntriesJson = {
        {"sender_id", this->candidate_id},
        {"message_type", APPEND_ENTRIES_MESSAGE},
        {"response", false},
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

    return append_entries_json.dump();
}

// Sets the state of the node based on passed string for convenience
void Raft_Node::setState(int state)
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

int Raft_Node::getCommitIndex(){
    return this->commitIndex;
}