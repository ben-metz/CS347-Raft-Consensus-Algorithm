// thread example
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>
#include <mutex>

#include "database.h"

class Server {
    private:
        std::thread* thread;
        Database *database;
        int id;
        void server_function();
        int* sockfd;
        struct sockaddr_in* msg_socket;

    public:
        Server(int id, int* sockfd, struct sockaddr_in* socket_address);
        int getID();
        void join();
        void send_to_client(std::string msg);
};