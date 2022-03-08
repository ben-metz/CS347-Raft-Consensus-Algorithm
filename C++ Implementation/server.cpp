#include "server.h"
#include "manager.h"
#include "database.h"
#include "raft.h"

using json = nlohmann::json;

std::mutex mtx;

std::mutex socket_mutex[SERVER_COUNT];

extern std::atomic_bool running_;

// Server initialiser
Server::Server() {}

// Initialise the variables used by the server
void Server::initialise(int id, Manager *manager,
                        int port, int server_socket_address_count)
{
    this->database = (Database *)malloc(sizeof(Database));
    *this->database = Database(DATABASE_SIZE);
    this->manager = manager;

    this->server_address_added = (int *)malloc(sizeof(int));
    *this->server_address_added = 0;

    this->socket_addr = (struct server_socket_address *)malloc(sizeof(struct server_socket_address));
    this->neighbours = (struct server_socket_address **)malloc(sizeof(struct server_socket_address *) * server_socket_address_count);

    this->raft = new Raft_Node(id, SERVER_COUNT, this);

    this->raft_response = (char *)malloc(sizeof(char) * 250);

    this -> stopped = 0;

    this->initSocket(port);

    this->initThread();
}

// Join the thread and close the receive socket
void Server::join()
{
    this->thread->join();

    std::cout << "Closing Server " << this->getID() << " Socket and Freeing Memory..."
              << "\n";
    close(*(this->socket_addr->fd));

    free(this->socket_addr->fd);

    free(this->rcv_buffer);
    free(this->rcv_n);
    free(this->rcv_socklen);

    free(this->neighbours);
    free(this->socket_addr);
    free(this->server_address_added);

    free(this->thread);

    free(this->database->get_size());
    free(this->database->get_data());
    free(this->database);

    delete this->raft;
    free(this->raft_response);
}

// Function to be performed by the server
// This thread will take messages sent by other servers,
// feed them into the algorithm, and reply/send to other servers
void Server::server_function()
{
    using namespace std::chrono;

    int flags = fcntl(*this->socket_addr->fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(*this->socket_addr->fd, F_SETFL, flags);

    while (running_)
    {
        // Print received messages to console
        *this->rcv_n = recvfrom(*this->socket_addr->fd, (char *)this->rcv_buffer, 1024,
                                MSG_WAITALL, (struct sockaddr *)&this->socket_addr->addr,
                                this->rcv_socklen);

        // If a message is available, handle it
        if (*this->rcv_n != -1)
        {
            this->rcv_buffer[*this->rcv_n] = '\0';

            //std::cout << "Server " << this->getID() << " Received Message: " << this->rcv_buffer << '\n';

            this->handleMessage(rcv_buffer);
        }
        
        // Check that the raft timer has not expired, if it has then request a vote from all servers
        if (*this->server_address_added >= EXPECTED_NEIGHBOURS)
        {
            this->raft->run();
        }

        // Reduce active waiting load by sleeping for 1ms
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Handles messages received on the receive socket
void Server::handleMessage(char *msg)
{
    json deserialised_json = json::parse(std::string(msg));

    // if (deserialised_json["message_type"] == "data_update")
    // {
    //     this->database->set_value(deserialised_json["data"]["index"], deserialised_json["data"]["value"]);
    //     this->send_details("Database Update");
    // }
    // else
    if (deserialised_json["message_type"] == "set_server_status")
    {
        this->set_status(deserialised_json["data"]["stopped"].get<int>());
        this->send_details("Status Change");
    }
    else if (this->stopped == 0)
    {
        this->raft->input_message(msg);
    }
}

void Server::set_status(int new_status){
    this->stopped = new_status;
}

// Function to send details to python client
void Server::send_details(std::string action)
{
    std::ostringstream ss;

    // Convert data to string
    for (int i = 0; i < *database->get_size(); i++)
    {
        ss << this->database->get_value(i) << ' ';
    }

    // Create json message
    json details_json = {
        {"message_type", "details_update"},
        {"data", {{"id", this->raft->getID()},       // candidate's term
                  {"state", this->raft->getState()}, // candidate's term
                  {"term", this->raft->getTerm()},   // candidate's term
                  {"vote", this->raft->getVote()},   // candidate requesting vote
                  {"action", action},
                  {"database", ss.str()}}}};

    mtx.lock();
    this->manager->send_msg(details_json.dump());
    mtx.unlock();
}

// Initialise the thread with the thread function
void Server::initThread()
{
    this->thread = (std::thread *)malloc(sizeof(std::thread));
    *this->thread = std::thread(&Server::server_function, this);
}

// Initialise the listening socket
void Server::initSocket(int port)
{
    struct sockaddr_in rcv_addr;

    this->rcv_buffer = (char *)malloc(sizeof(char) * 1024);
    this->rcv_n = (int *)malloc(sizeof(int));
    this->rcv_socklen = (socklen_t *)malloc(sizeof(socklen_t));
    *this->rcv_socklen = sizeof(rcv_addr);
    int *receive_socket_fd = (int *)malloc(sizeof(int));

    // Creating socket file descriptor
    if ((*receive_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Receive socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    rcv_addr.sin_family = AF_INET;
    rcv_addr.sin_port = htons(port);
    rcv_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket with the server address
    if (bind(*receive_socket_fd,
             (const struct sockaddr *)&rcv_addr,
             sizeof(rcv_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    this->socket_addr->server_socket_address_id = this->getID();
    this->socket_addr->fd = receive_socket_fd;
    this->socket_addr->addr = rcv_addr;
}

// Adds this socket to the server_socket_addresss of the other servers
void Server::addToNeighbours()
{
    this->manager->addSocket(this->socket_addr);
}

// Return the ID of server
int Server::getID()
{
    return this->raft->getID();
}

// Adds a socket to the server_socket_addresss array of the server
void Server::addSocket(struct server_socket_address *addr)
{
    this->neighbours[*this->server_address_added] = addr;

    *this->server_address_added += 1;
}

// Sends message to the server with the specified ID
void Server::sendToServer(int id, std::string msg)
{
    if (this -> stopped == 0){
        int sock_id = this->getSocketIndex(id);

        while (socket_mutex[id].try_lock())
            ;

        sendto(*(this->neighbours[sock_id]->fd), (const char *)msg.c_str(), strlen(msg.c_str()),
            MSG_CONFIRM, (const struct sockaddr *)&(this->neighbours[sock_id]->addr),
            sizeof(this->neighbours[sock_id]->addr));

        socket_mutex[id].unlock();
    }
}

// Sends message to all of the servers
void Server::sendToAllServers(std::string msg)
{
    if (this -> stopped == 0){
        for (int i = 0; i < EXPECTED_NEIGHBOURS; i++)
        {
            this->sendToServer(this->neighbours[i]->server_socket_address_id, msg);
        }
    }
}

// Get index of socket in neighbours for server with passed ID
int Server::getSocketIndex(int server_id)
{
    if (server_id > this->getID())
    {
        return server_id - 1;
    }
    else
    {
        return server_id;
    }
}

// Returns the servers receiving socket in a server_socket_address struct
struct server_socket_address *Server::getSocket()
{
    return this->socket_addr;
}

int Server::getServerSocketAddress(int server)
{
    if (server >= EXPECTED_NEIGHBOURS)
        server = EXPECTED_NEIGHBOURS - 1;

    return this->neighbours[server]->server_socket_address_id;
}