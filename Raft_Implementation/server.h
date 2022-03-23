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
    int serverSocketAddressId;
    int *fd;
    struct sockaddr_in addr;
};

class Server
{
private:
    void serverFunction();

    int serverId;
    Manager *manager;
    char *rcvBuffer;
    int *rcvN;
    socklen_t *rcvSocklen;
    struct server_socket_address **neighbours;
    int *serverAddressAdded;
    int getSocketIndex(int serverId);

    struct server_socket_address *socketAddr;
    std::thread thread;
    Database *database;
    
    void handleMessage(char *msg);

    int stopped;
    void setStatus(int newStatus);

    Raft_Node *raft;
    char *raftResponse;
public:
    Server();
    ~Server();

    int getID();
    void initialise(int id, Manager *manager,
                    int port, int serverSocketAddressCount);
    void initSocket(int port);
    void addSocket(struct server_socket_address *addr);
    void addToNeighbours();
    struct server_socket_address *getSocket();
    void sendToAllServers(std::string msg);
    void sendToServer(int id, std::string msg);
    void sendDetails(std::string action);
    int getServerSocketAddress(int server);

    inline bool isRunning() { return !stopped; }

    inline Database* getDatabase() { return database; }

    std::thread* getThread();
};