#include "server.h"
#include "manager.h"
#include "database.h"
#include "raft.h"

using json = nlohmann::json;

std::mutex mtx;

std::mutex socketMutex[SERVER_COUNT];

extern std::atomic_bool running_;

// Server initialiser
Server::Server() {}

Server::~Server()
{
   std::cout << "Closing Server " << this->getID() << " Socket and Freeing Memory..."
              << "\n";
    close(*(this->socketAddr->fd));

    free(this->socketAddr->fd);
    this->socketAddr->fd = nullptr;

    free(this->rcvBuffer);
    this->rcvBuffer = nullptr;
    free(this->rcvN);
    this->rcvN = nullptr;
    free(this->rcvSocklen);
    this->rcvSocklen = nullptr;

    free(this->neighbours);
    this->neighbours = nullptr;
    free(this->socketAddr);
    this->socketAddr = nullptr;
    free(this->serverAddressAdded);
    this->serverAddressAdded = nullptr;

    delete this->database;
    this->database = nullptr;

    delete this->raft;
    this->raft = nullptr;
    free(this->raftResponse);
    this->raftResponse = nullptr;
}

// Initialise the variables used by the server
void Server::initialise(int id, Manager *manager,
                        int port, int server_socket_address_count)
{
    serverId = id;

    this->database = new Database(DATABASE_SIZE);
    this->manager = manager;

    this->serverAddressAdded = (int *)malloc(sizeof(int));
    *this->serverAddressAdded = 0;

    this->socketAddr = (struct server_socket_address *)malloc(sizeof(struct server_socket_address));
    this->neighbours = (struct server_socket_address **)malloc(sizeof(struct server_socket_address *) * server_socket_address_count);

    this->raft = new RaftNode(id, SERVER_COUNT, this);

    this->raftResponse = (char *)malloc(sizeof(char) * 250);

    this->stopped = 0;

    this->initSocket(port);

    this->thread = std::thread(&Server::serverFunction, this);
}

std::thread* Server::getThread()
{
    return &this -> thread;
}

// Function to be performed by the server
// This thread will take messages sent by other servers,
// feed them into the algorithm, and reply/send to other servers
void Server::serverFunction()
{
    using namespace std::chrono;

    int flags = fcntl(*this->socketAddr->fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(*this->socketAddr->fd, F_SETFL, flags);

    while (running_)
    {
        // Attempt to receive from the socket
        *this->rcvN = recvfrom(*this->socketAddr->fd, (char *)this->rcvBuffer, 1024,
                                MSG_WAITALL, (struct sockaddr *)&this->socketAddr->addr,
                                this->rcvSocklen);

        // If a message is available, handle it
        if (*this->rcvN != -1)
        {
            this->rcvBuffer[*this->rcvN] = '\0';

            //std::cout << "Server " << this->getID() << " Received Message: " << this->rcvBuffer << '\n';

            this->handleMessage(rcvBuffer);
        }
        
        // If all servers have been added and the node isn't stopped then run the raft algorithm
        if (*this->serverAddressAdded >= EXPECTED_NEIGHBOURS && !this->stopped)
        {
            this->raft->run();
        }

        // Reduce active waiting load by sleeping for 1ms
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

// Handles messages received on the receive socket
void Server::handleMessage(char *msg)
{
    json deserialisedJson = json::parse(std::string(msg));

    if (deserialisedJson["message_type"] == "set_server_status")
    {
        this->setStatus(deserialisedJson["data"]["stopped"].get<int>());

        this->raft->resetElectionTimer();
    }
    else if (deserialisedJson["message_type"] == "state_injection")
    {
        if (deserialisedJson["data"]["new_state"] == "Leader"){
            this->raft->setState(LEADER); 
        } else if (deserialisedJson["data"]["new_state"] == "Candidate"){
            this->raft->setState(CANDIDATE); 
        } else if (deserialisedJson["data"]["new_state"] == "Follower"){
            this->raft->setState(FOLLOWER); 
        }

        this->raft->setTimeout(deserialisedJson["data"]["election_timeout"].get<int>());

        this->raft->setTerm(deserialisedJson["data"]["term"].get<int>());
    }
    else if (this->stopped == 0)
    {
        this->raft->inputMessage(msg);
    }
}

void Server::setStatus(int newStatus){
    this->stopped = newStatus;

    if (newStatus == 0){
        this->sendDetails("Status Change: Restarted");
    } else {
        this->sendDetails("Status Change: Halted");
    }
}

// Function to send details to python client
void Server::sendDetails(std::string action)
{
    std::ostringstream ss;

    // Convert data to string
    for (int i = 0; i < database->getSize(); i++)
    {
        ss << this->database->getValue(i) << ' ';
    }

    // Create json message
    json detailsJson = {
        {"message_type", "details_update"},
        {"data", {{"id", this->raft->getID()},       // candidate's term
                  {"state", this->raft->getState()}, // candidate's term
                  {"term", this->raft->getTerm()},   // candidate's term
                  {"vote", this->raft->getVote()},   // candidate requesting vote
                  {"action", action},
                  {"lastCommited", this->raft->getCommitIndex()},
                  {"database", ss.str()}}}};

    mtx.lock();
    this->manager->sendMsg(detailsJson.dump());
    mtx.unlock();
}

// Initialise the listening socket
void Server::initSocket(int port)
{
    struct sockaddr_in rcvAddr;

    this->rcvBuffer = (char *)malloc(sizeof(char) * 1024);
    this->rcvN = (int *)malloc(sizeof(int));
    this->rcvSocklen = (socklen_t *)malloc(sizeof(socklen_t));
    *this->rcvSocklen = sizeof(rcvAddr);
    int *receiveSocketFd = (int *)malloc(sizeof(int));

    // Creating socket file descriptor
    if ((*receiveSocketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Receive socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    rcvAddr.sin_family = AF_INET;
    rcvAddr.sin_port = htons(port);
    rcvAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket with the server address
    if (bind(*receiveSocketFd,
             (const struct sockaddr *)&rcvAddr,
             sizeof(rcvAddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    this->socketAddr->serverSocketAddressId = this->getID();
    this->socketAddr->fd = receiveSocketFd;
    this->socketAddr->addr = rcvAddr;
}

// Adds this socket to the server_socket_addresss of the other servers
void Server::addToNeighbours()
{
    this->manager->addSocket(this->socketAddr);
}

// Return the ID of server
int Server::getID()
{
    return serverId;
}

// Adds a socket to the server_socket_addresss array of the server
void Server::addSocket(struct server_socket_address *addr)
{
    this->neighbours[*this->serverAddressAdded] = addr;

    *this->serverAddressAdded += 1;
}

// Sends message to the server with the specified ID
void Server::sendToServer(int id, std::string msg)
{
    if (this -> stopped == 0){
        int sockId = this->getSocketIndex(id);

        // Block until the mutex isn't taken
        while (socketMutex[id].try_lock())
            ;

        // Send the message to the server
        sendto(*(this->neighbours[sockId]->fd), (const char *)msg.c_str(), strlen(msg.c_str()),
            MSG_CONFIRM, (const struct sockaddr *)&(this->neighbours[sockId]->addr),
            sizeof(this->neighbours[sockId]->addr));

        // Release the mutex
        socketMutex[id].unlock();
    }
}

// Sends message to all of the servers
void Server::sendToAllServers(std::string msg)
{
    if (this->stopped == 0)
    {
        for (int i = 0; i < EXPECTED_NEIGHBOURS; i++)
        {
            this->sendToServer(this->neighbours[i]->serverSocketAddressId, msg);
        }
    }
}

// Get index of socket in neighbours for server with passed ID
int Server::getSocketIndex(int serverId)
{
    if (serverId > this->getID())
    {
        return serverId - 1;
    }
    else
    {
        return serverId;
    }
}

// Returns the servers receiving socket in a server_socket_address struct
struct server_socket_address *Server::getSocket()
{
    return this->socketAddr;
}

int Server::getServerSocketAddress(int server)
{
    if (server > this->raft->getID())
        server--;

    return this->neighbours[server]->serverSocketAddressId;
}