class Raft_Node{
    private:
        int state;
        /* If a database change is received, the update command 
        is stored here to be sent to the database stored on 
        the server */
        char** database_change_buffer; 

    public:
        Raft_Node();
        void input_message(char* msg, char* output_buffer);
};