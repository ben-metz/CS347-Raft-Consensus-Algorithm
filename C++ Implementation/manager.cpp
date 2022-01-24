#include "manager.h"
#include "server.h"
#include <unistd.h>
#include <atomic>

std::atomic_bool running {true};

// Manager constructor, define socket and servers
Manager::Manager(){
    this -> init_socket();
    this -> init_servers();
}

// Initialise the Python communication socket
void Manager::init_socket(){
    this -> sockfd = (int*) malloc(sizeof(int));

    // Creating socket file descriptor
    if ( (*(this -> sockfd) = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    this -> servaddr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
       
    // Filling server information
    this -> servaddr -> sin_family = AF_INET;
    this -> servaddr -> sin_port = htons(PORT);
    this -> servaddr -> sin_addr.s_addr = INADDR_ANY;

    this -> send_msg("Connection Started...");
}

// Initialise the servers
void Manager::init_servers(){
    this -> servers = (Server*) malloc(sizeof(Server) * SERVER_COUNT);
  
    // Initialise the server threads
    for(int i = 0; i < SERVER_COUNT; i++){
        this -> servers[i] = Server();
    }

    for(int i = 0; i < SERVER_COUNT; i++){
        this -> servers[i].initialise(i, this, std::ref(running));
    }
}

// When completed, join server threads and free stuff (not everything freed yet, bug fixing :( )
void Manager::finish(){
    running = false;

    std::cout << "Joining Threads...\n";
    for(int i = 0; i < SERVER_COUNT; i++){
        this -> servers[i].join();
    }
    
    this -> send_msg("Connection Ended...");

    std::cout << "Closing Socket...\n";
    close(*(this -> sockfd));

    std::cout << "Freeing Dynamic Memory...\n";
    free(this -> sockfd);
    free(this -> servaddr);
}

// Sends a message back to the client
void Manager::send_msg(std::string msg) 
{
    sendto(*(this -> sockfd), (const char *)msg.c_str(), strlen(msg.c_str()),
        MSG_CONFIRM, (const struct sockaddr *) this -> servaddr, 
            sizeof(*(this -> servaddr)));
}
