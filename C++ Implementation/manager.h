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
#include <atomic>

#define SERVER_COUNT        6
#define IP                  "127.0.0.1" // Loopback
#define SEND_PORT           12345 // Port to send details to Python
#define RCV_PORT            12346 // Port to receive things commands Python
#define SERVER_START_PORT   12347 // Port for inter-server communication

class Server;

class Manager {
    private:
        void init_sockets();
        void init_servers(int updates_per_second);
        void init_listener();
        void listener_function(std::atomic<bool>& running);
        char* rcv_buffer;
        int* rcv_n;
        socklen_t* rcv_socklen;
        std::atomic_bool* running_;
    
    public:
        Manager();
        void initialise(int updates_per_second);
        void send_msg(std::string msg);
        void finish();
        int *receive_socket_fd;
        int *send_socket_fd;
        struct sockaddr_in send_addr;
        struct sockaddr_in rcv_addr;
        Server* servers;
        std::thread* listener;
};
