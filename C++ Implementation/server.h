#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <fcntl.h>

class Database;

class Manager;

// Stores neighbour server communication details
struct neighbour {
    int neighbour_id;
    int* fd;
    struct sockaddr_in addr;
};

class Server {
    private:
        int id;
        void server_function(std::atomic<bool>& running);
        void initThread(std::atomic<bool>& running);
        Manager* manager;
        char* rcv_buffer;
        int* rcv_n;
        socklen_t* rcv_socklen;
        struct neighbour* neighbours;
        int* neighbours_added;

    public:
        struct sockaddr_in rcv_addr;
        int *receive_socket_fd;
        std::thread* thread;
        Database *database;
        Server();
        int getID();
        void join();
        void initialise(int id, Manager* manager, std::atomic<bool>& running, 
            unsigned long long next_time, int delay, int port, int neighbour_count);
        unsigned long long* next_time;
        int* delay; // Delay between update messages
        void initSocket(int port);
        void send_details();
        void addSocket(int neighbour_id, int* fd, struct sockaddr_in addr);
        void addToNeighbours();
        void sendToServer(int id, std::string msg);
};