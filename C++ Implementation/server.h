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
        int id;
        void server_function();
        int* sockfd;
        struct sockaddr_in* msg_socket;
        void initThread();

    public:
        std::thread* thread;
        Database *database;
        Server(int id, int* sockfd, struct sockaddr_in* socket_address);
        int getID();
        void join();
        void diagnostic();
        void send_to_client(std::string msg);
};