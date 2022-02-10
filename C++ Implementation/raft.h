#include <string>

class Server;

class Raft_Node
{
private:
    int state;

    /* If a database change is received, the update command 
        is stored here to be sent to the database stored on 
        the server */
    char **database_change_buffer;
    int random_timeout;
    long *time_of_last_message;

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
    void input_message(char *msg, char *output_buffer);
    bool checkTimer();

    std::string getVoteRequestMessage(int last_log_index = -1, int last_log_term = -1);
    std::string getVoteResponseMessage(bool voteGranted);

    int getRandomTimeout();
    int getID();
    int getState();
    int getTerm();
    int getVote();

    void setState(std::string state);
};