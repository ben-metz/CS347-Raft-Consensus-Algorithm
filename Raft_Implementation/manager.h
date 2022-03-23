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
#define SEND_PORT_BACKUP 12344  // Port to send details to Node.js
#define SEND_PORT 12345         // Port to send details to Python
#define RCV_PORT 12346          // Port to receive commands from Python
#define SERVER_START_PORT 12347 // Ports for inter-server communication

// (end port = SERVER_START_PORT + SERVER_COUNT)

class Server;

class Manager
{
private:
    char *rcvBuffer;
    int *rcvN;
    socklen_t *rcvSocklen;
    int *receiveSocketFd;
    int *sendSocketFd;
    int *sendSocketBackupFd;
    struct sockaddr_in sendAddr;
    struct sockaddr_in sendAddrBackup;
    struct sockaddr_in rcvAddr;
    Server **servers;
    std::thread listener;

    void initSockets();
    void initServers();
    void initListener();
    void listenerFunction();

    void handleMessage(char *msg, int len);
    void sendToAllServers(char *msg, int len);
    void sendToServer(int id, char *msg, int len);
    struct server_socket_address *serverAddresses;
    void updateServerValue(int *updateProperties);

public:
    Manager();
    ~Manager();

    void sendMsg(std::string msg);
    void addSocket(struct server_socket_address *addr);
};
