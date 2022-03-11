#pragma once

#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "json.hpp"

#define DATABASE_SIZE 5
#define EXPECTED_NEIGHBOURS 4

class Database;

class Manager;

class Raft_Node;

// Stores server_socket_address server communication details
struct server_socket_address
{
    int server_socket_address_id;
    int *fd;
    struct sockaddr_in addr;
};

class Server
{
private:
    void server_function();

    int server_id;
    Manager *manager;
    char *rcv_buffer;
    int *rcv_n;
    socklen_t *rcv_socklen;
    struct server_socket_address **neighbours;
    int *server_address_added;
    int getSocketIndex(int server_id);

    struct server_socket_address *socket_addr;
    std::thread thread;
    Database *database;
    
    void handleMessage(char *msg);

    int stopped;
    void set_status(int new_status);
    void set_timeout(int new_timeout);

    int delayMs;

    Raft_Node *raft;
    char *raft_response;
public:
    Server();
    ~Server();

    int getID();
    void initialise(int id, Manager *manager,
                    int port, int server_socket_address_count);
    void initSocket(int port);
    void addSocket(struct server_socket_address *addr);
    void addToNeighbours();
    struct server_socket_address *getSocket();
    void sendToAllServers(std::string msg);
    void sendToServer(int id, std::string msg);
    void send_details(std::string action);
    int getServerSocketAddress(int server);

    inline bool isRunning() { return !stopped; }

    inline Database* getDatabase() { return database; }

    std::thread* getThread();
};