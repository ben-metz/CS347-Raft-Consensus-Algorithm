#include "manager.h"
#include "server.h"
#include <unistd.h>
#include <chrono>
#include <fcntl.h>

// Manager constructor, define socket and servers
Manager::Manager(){}

// Initialise all components of the manager
void Manager::initialise(int updates_per_second){
    this -> running_ = (std::atomic_bool*) malloc(sizeof(std::atomic_bool));
    *this -> running_ = true;

    this -> init_sockets();

    this -> init_listener();
    
    this -> init_servers(updates_per_second);
}

// Initialise the Python communication sockets
void Manager::init_sockets(){
    this -> receive_socket_fd = (int*) malloc(sizeof(int));
    this -> send_socket_fd = (int*) malloc(sizeof(int));

    // Creating socket file descriptor
    if ( (*(this -> receive_socket_fd) = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("Receive socket creation failed");
        exit(EXIT_FAILURE);
    }

    if ( (*(this -> send_socket_fd) = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("Send socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    this -> send_addr.sin_family = AF_INET;
    this -> send_addr.sin_port = htons(SEND_PORT);
    this -> send_addr.sin_addr.s_addr = INADDR_ANY;

    this -> rcv_addr.sin_family = AF_INET;
    this -> rcv_addr.sin_port = htons(RCV_PORT);
    this -> rcv_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket with the server address
    if ( bind(*receive_socket_fd, 
            (const struct sockaddr *) &rcv_addr, 
            sizeof(rcv_addr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    this -> send_msg("Connection Started...");
}

// Function that listens to the receive socket
void Manager::listener_function(std::atomic<bool>& running){
    int flags = fcntl(*receive_socket_fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(*receive_socket_fd, F_SETFL, flags);

    while(running){
        *this -> rcv_n = recvfrom(*receive_socket_fd, (char *) this -> rcv_buffer, 1024, 
            MSG_WAITALL, ( struct sockaddr *) &rcv_addr,
            this -> rcv_socklen);

        if (*this -> rcv_n != -1){
            this -> rcv_buffer[*this -> rcv_n] = '\0';

            std::cout << this -> rcv_buffer << '\n';
        }
    }

    std::cout << "Exited Successfully";
}

// Initialise update listener
void Manager::init_listener(){
    this -> rcv_buffer = (char*) malloc(sizeof(char) * 1024);
    this -> rcv_n = (int*) malloc(sizeof(int));
    this -> rcv_socklen = (socklen_t*) malloc(sizeof(socklen_t));
    *this -> rcv_socklen = sizeof(this -> rcv_addr);

    this -> listener = (std::thread*) malloc(sizeof(std::thread));
    *this -> listener = std::thread(&Manager::listener_function, this, std::ref(*this -> running_)); 
}

// Initialise the servers
void Manager::init_servers(int updates_per_second){
    this -> servers = (Server*) malloc(sizeof(Server) * SERVER_COUNT);
  
    // Initialise the server threads
    for(int i = 0; i < SERVER_COUNT; i++){
        this -> servers[i] = Server();
    }

    using namespace std::chrono;
    unsigned long long ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    for(int i = 0; i < SERVER_COUNT; i++){
        this -> servers[i].initialise(i, this, std::ref(*this -> running_), ms + i * ((1000/updates_per_second)/SERVER_COUNT), 1000/updates_per_second);
    }
}

// When completed, join server threads and free stuff (not everything freed yet, bug fixing :( )
void Manager::finish(){
    *this -> running_ = false;

    std::cout << "Joining Threads...\n";
    
    for(int i = 0; i < SERVER_COUNT; i++){
        this -> servers[i].join();
    }

    this -> listener -> join();

    free(this -> listener);
    free(this -> servers);

    this -> send_msg("Connection Ended...");

    std::cout << "Closing Socket...\n";
    close(*(this -> receive_socket_fd));
    close(*(this -> send_socket_fd));

    std::cout << "Freeing Dynamic Memory...\n";
    free(this -> receive_socket_fd);
    free(this -> send_socket_fd);

    free(this -> rcv_buffer);
    free(this -> rcv_n);
    free(this -> rcv_socklen);
}

// Sends a message back to the client
void Manager::send_msg(std::string msg) 
{
    sendto(*(this -> receive_socket_fd), (const char *)msg.c_str(), strlen(msg.c_str()),
        MSG_CONFIRM, (const struct sockaddr *) &(this -> send_addr), 
            sizeof(this -> send_addr));
}
