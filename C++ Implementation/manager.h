#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sstream>
#include <mutex>
#include <atomic>
#include <fcntl.h>

#define SERVER_COUNT        6
#define IP                  "127.0.0.1" // Loopback
#define SEND_PORT           12345 // Port to send details to Python
#define RCV_PORT            12346 // Port to receive things commands Python
#define SERVER_START_PORT   12347 // Port for inter-server communication

class Server;

class Manager {
    private:
        char* rcv_buffer;
        int* rcv_n;
        socklen_t* rcv_socklen;
        std::atomic_bool* running_;
        int *receive_socket_fd;
        int *send_socket_fd;
        struct sockaddr_in send_addr;
        struct sockaddr_in rcv_addr;
        Server* servers;
        std::thread* listener;

        void init_sockets();
        void init_servers(int updates_per_second);
        void init_listener();
        void listener_function(std::atomic<bool>& running);

        void handle_message(char* msg, int len);
        void send_to_all_servers(char* msg, int len);
        void send_to_server(int id, char* msg, int len);
        struct neighbour* server_addresses;
        void update_server_value(int* update_properties);
    public:
        Manager();
        void initialise(int updates_per_second);
        void send_msg(std::string msg);
        void finish();
        void addSocket(int id, int* fd, struct sockaddr_in sock_addr);
};
