#include "manager.h"
#include "server.h"

using json = nlohmann::json;

std::atomic_bool running_(true);

// Manager constructor, define socket and servers
Manager::Manager()
    : listener{}
{
    this->serverAddresses = (struct server_socket_address *)malloc(sizeof(struct server_socket_address) * SERVER_COUNT);

    this->initSockets();

    this->initListener();

    this->initServers();
}

Manager::~Manager()
{
    running_ = false; // Make threads exit their functions

    // Join the python listener thread if possible
    if (this->listener.joinable() && this->listener.get_id() != std::this_thread::get_id())
        this->listener.join();
    std::cout << "Joined Python Listener Thread\n";

    // Join the server threads
    for (int i = 0; i < SERVER_COUNT; i++)
    {
        std::thread* serverThread = this->servers[i]->getThread();
        if (serverThread->joinable() && serverThread->get_id() != std::this_thread::get_id() && this->servers[i]->isRunning())
            serverThread->join();

        std::cout << "Joined Server " << i << " Thread\n";
    }

    // Delete server instances
    for (int i = 0; i < SERVER_COUNT; i++)
    {
        delete this->servers[i];
        this->servers[i] = nullptr;
    }

    free(this->servers);
    this->servers = nullptr;

    json endMsg = {
        {"message_type", "connection_status"},
        {"data", "ended"}};

    this->sendMsg(endMsg.dump());

    std::cout << "Closing Python Communication Sockets and Freeing Memory...\n";
    close(*(this->receiveSocketFd));
    close(*(this->sendSocketFd));

    free(this->receiveSocketFd);
    this->receiveSocketFd = nullptr;
    free(this->sendSocketFd);
    this->sendSocketFd = nullptr;
    free(this->sendSocketBackupFd);
    this->sendSocketBackupFd = nullptr;

    free(this->rcvBuffer);
    this->rcvBuffer = nullptr;
    free(this->rcvN);
    this->rcvN = nullptr;
    free(this->rcvSocklen);
    this->rcvSocklen = nullptr;

    free(this->serverAddresses);
    this->serverAddresses = nullptr;
}

// Initialise the Python communication sockets
void Manager::initSockets()
{
    this->receiveSocketFd = (int *)malloc(sizeof(int));
    this->sendSocketFd = (int *)malloc(sizeof(int));
    this->sendSocketBackupFd = (int *)malloc(sizeof(int));

    // Creating socket file descriptor
    if ((*(this->receiveSocketFd) = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Receive socket creation failed");
        exit(EXIT_FAILURE);
    }

    if ((*(this->sendSocketFd) = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Send socket creation failed");
        exit(EXIT_FAILURE);
    }

    if ((*(this->sendSocketBackupFd) = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Send socket for Node.js creation failed");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    this->sendAddr.sin_family = AF_INET;
    this->sendAddr.sin_port = htons(SEND_PORT);
    this->sendAddr.sin_addr.s_addr = INADDR_ANY;

    // Filling server information
    this->sendAddrBackup.sin_family = AF_INET;
    this->sendAddrBackup.sin_port = htons(SEND_PORT_BACKUP);
    this->sendAddrBackup.sin_addr.s_addr = INADDR_ANY;

    this->rcvAddr.sin_family = AF_INET;
    this->rcvAddr.sin_port = htons(RCV_PORT);
    this->rcvAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket with the server address
    if (bind(*receiveSocketFd,
             (const struct sockaddr *)&rcvAddr,
             sizeof(rcvAddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    json startMsg = {
        {"message_type", "connection_status"},
        {"data", "started"}};

    this->sendMsg(startMsg.dump());
}

// Function that listens to the receive socket
void Manager::listenerFunction()
{
    int flags = fcntl(*receiveSocketFd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(*receiveSocketFd, F_SETFL, flags);

    while (running_)
    {
        *this->rcvN = recvfrom(*receiveSocketFd, (char *)this->rcvBuffer, 1024,
                                MSG_WAITALL, (struct sockaddr *)&rcvAddr,
                                this->rcvSocklen);

        if (*this->rcvN != -1)
        {
            this->rcvBuffer[*this->rcvN] = '\0';

            this->handleMessage(this->rcvBuffer, *this->rcvN + 1);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Handles messages received from the Python client
void Manager::handleMessage(char *msg, int len)
{
    json deserialisedJson = json::parse(std::string(msg));

    if (deserialisedJson["message_type"] == "data_update")
    {
        this->sendToServer(deserialisedJson["data"]["server_id"].get<int>(), msg, len);
    }
    else if (deserialisedJson["message_type"] == "set_server_status")
    {
        this->sendToServer(deserialisedJson["data"]["server_id"].get<int>(), msg, len);
    }
    else if (deserialisedJson["message_type"] == "state_injection")
    {
        this->sendToServer(deserialisedJson["data"]["server_id"].get<int>(), msg, len);
    }
}

// Sends a message to the server specified by the id
void Manager::sendToServer(int id, char *msg, int len)
{
    sendto(*(this->serverAddresses[id].fd), (const char *)msg, len,
           MSG_CONFIRM, (const struct sockaddr *)&(this->serverAddresses[id].addr),
           sizeof(this->serverAddresses[id].addr));
}

// Sends a message to all servers
void Manager::sendToAllServers(char *msg, int len)
{
    for (int i = 0; i < SERVER_COUNT; i++)
    {
        sendto(*(this->serverAddresses[i].fd), (const char *)msg, len,
               MSG_CONFIRM, (const struct sockaddr *)&(this->serverAddresses[i].addr),
               sizeof(this->serverAddresses[i].addr));
    }
}

// Initialise update listener
void Manager::initListener()
{
    this->rcvBuffer = (char *)malloc(sizeof(char) * 1024);
    this->rcvN = (int *)malloc(sizeof(int));
    this->rcvSocklen = (socklen_t *)malloc(sizeof(socklen_t));
    *this->rcvSocklen = sizeof(this->rcvAddr);

    this->listener = std::thread(&Manager::listenerFunction, this);
}

// Initialise the servers
void Manager::initServers()
{
    this->servers = (Server **)malloc(sizeof(Server*) * SERVER_COUNT);

    // Initialise the server threads
    for (int i = 0; i < SERVER_COUNT; i++)
    {
        this->servers[i] = new Server();
    }

    using namespace std::chrono;
    unsigned long long ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    // Allocate memory, start thread, etc
    for (int i = 0; i < SERVER_COUNT; i++)
    {
        this->servers[i]->initialise(i, this, SERVER_START_PORT + i, SERVER_COUNT - 1);
        this->serverAddresses[i] = *this->servers[i]->getSocket();
    }

    // Share the sockets to all servers
    for (int i = 0; i < SERVER_COUNT; i++)
    {
        this->servers[i]->addToNeighbours();
    }
}

// Sends a message back to the client
void Manager::sendMsg(std::string msg)
{  
    sendto(*(this->sendSocketFd), (const char *)msg.c_str(), strlen(msg.c_str()),
           MSG_CONFIRM, (const struct sockaddr *)&(this->sendAddr),
           sizeof(this->sendAddr));
    sendto(*(this->sendSocketBackupFd), (const char *)msg.c_str(), strlen(msg.c_str()),
           MSG_CONFIRM, (const struct sockaddr *)&(this->sendAddrBackup),
           sizeof(this->sendAddrBackup));
}

// Add socket to server_socket_address arrays for all servers except server with passed id
void Manager::addSocket(struct server_socket_address *addr)
{
    for (int i = 0; i < SERVER_COUNT; i++)
    {
        if (i != addr->serverSocketAddressId)
        {
            servers[i]->addSocket(addr);
        }
    }
}
