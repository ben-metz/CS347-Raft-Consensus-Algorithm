#include <string>

#define LEADER 0
#define CANDIDATE 1
#define FOLLOWER 2

#define HEARTBEAT_TIMEOUT 100

class Server;

class Raft_Node
{
private:
    int state;

    /* If a database change is received, the update command 
        is stored here to be sent to the database stored on 
        the server */
    char **database_change_buffer;

    int election_timeout;
    int heartbeat_timeout;

    long *time_of_last_message;
    long *time_of_last_heartbeat;

    int server_count;

    int term;
    int candidate_id;
    int leader_id;

    // Leader election stuff
    int voted_for_id;
    int vote_count;

    Server *server; // Associated server

public:
    Raft_Node(int id, int server_count, Server *server);
    void run();
    void input_message(char *msg);

    bool checkElectionTimer();
    bool checkHeartbeatTimer();

    int getElectionTimeout();

    std::string getVoteRequestMessage(int last_log_index = -1, int last_log_term = -1);
    std::string getVoteResponseMessage(bool voteGranted);

    std::string getAppendEntriesMessage(bool heartbeat, int last_log_index = -1, int last_log_term = -1);
    std::string getAppendEntriesResponseMessage(bool success);

    int getID();
    int getState();
    int getTerm();
    int getVote();

    void setState(int state);
};