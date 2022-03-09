#pragma once

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

#define SERVER_COUNT 5
#define IP "127.0.0.1"          // Loopback
#define SEND_PORT 12345         // Port to send details to Python
#define RCV_PORT 12346          // Port to receive things commands Python
#define SERVER_START_PORT 12347 // Ports for inter-server communication

// (end port = SERVER_START_PORT + SERVER_COUNT)

class Server;

class Manager
{
private:
    char *rcv_buffer;
    int *rcv_n;
    socklen_t *rcv_socklen;
    int *receive_socket_fd;
    int *send_socket_fd;
    struct sockaddr_in send_addr;
    struct sockaddr_in rcv_addr;
    Server **servers;
    std::thread *listener;

    void init_sockets();
    void init_servers(int updates_per_second);
    void init_listener();
    void listener_function();

    void handle_message(char *msg, int len);
    void send_to_all_servers(char *msg, int len);
    void send_to_server(int id, char *msg, int len);
    struct server_socket_address *server_addresses;
    void update_server_value(int *update_properties);

public:
    Manager();
    ~Manager();

    void initialise(int updates_per_second);
    void send_msg(std::string msg);
    void finish();
    void addSocket(struct server_socket_address *addr);
};
