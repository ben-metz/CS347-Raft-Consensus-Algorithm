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

#include "server.h"

#define SERVER_COUNT 7
#define MAXLINE     1024 // Socket buffer size
#define IP          "127.0.0.1" // Loopback
#define PORT        12345

class Manager {
    private:
        int *sockfd;
        char* buffer;
        struct sockaddr_in*     servaddr;
        Server* servers;
        void init_socket();
        void init_servers();

    public:
        Manager();
        void send_msg(std::string msg);
        void finish();
};
