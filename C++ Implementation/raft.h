#include <string>

class Raft_Node{
    private:
        int state;
        /* If a database change is received, the update command 
        is stored here to be sent to the database stored on 
        the server */
        char** database_change_buffer; 
        int random_timeout;
        long* time_of_last_message;

        int term;
        int candidate_id;
        int leader_id;

    public:
        Raft_Node(int id);
        void input_message(char* msg, char* output_buffer);
        bool checkTimer();
        std::string getVoteRequestMessage(int last_log_index = -1, int last_log_term = -1);
        int getRandomTimeout();
        int getID();
};