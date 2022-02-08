#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "raft.h"

#define DATABASE_SIZE 5

class Database;

class Manager;

// Stores server_socket_address server communication details
struct server_socket_address {
    int server_socket_address_id;
    int* fd;
    struct sockaddr_in addr;
};

class Server {
    private:
        int id;
        void server_function();
        void initThread();
        Manager* manager;
        char* rcv_buffer;
        int* rcv_n;
        socklen_t* rcv_socklen;
        struct server_socket_address** neighbours;
        int* server_address_added;
        int getSocketIndex(int server_id);

        struct server_socket_address* socket_addr;
        std::thread* thread;
        Database *database;
        unsigned long long* next_time;
        int* delay; // Delay between update messages
        void send_details();
        void sendToServer(int id, std::string msg);
        void handleMessage(char* msg);

        Raft_Node* raft;
        char* raft_response;

    public:
        Server();
        int getID();
        void join();
        void initialise(int id, Manager* manager,
            unsigned long long next_time, int delay, int port, int server_socket_address_count);
        void initSocket(int port);
        void addSocket(struct server_socket_address* addr);
        void addToNeighbours();
        struct server_socket_address* getSocket();
};