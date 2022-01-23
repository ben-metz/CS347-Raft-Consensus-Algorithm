#include "manager.h"
#include <unistd.h>

Manager::Manager(){
    this -> buffer = (char*) malloc(sizeof(char) * MAXLINE);
    this -> init_socket();
    this -> init_servers();
}

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
}

void Manager::init_servers(){
    this -> servers = (Server*) malloc(sizeof(Server) * SERVER_COUNT);

    unsigned int microsecond = 10000;
    usleep(3 * microsecond);
  
    // Initialise the server threads
    for(int i = 0; i < SERVER_COUNT; i++){
        usleep(3 * microsecond);
        this -> servers[i] = Server(i, this -> sockfd, this -> servaddr); 
    }
}

void Manager::finish(){
    for(int i = 0; i < SERVER_COUNT; i++){
        this -> servers[i].join();
    }

    close(*(this -> sockfd));

    free(this -> sockfd);
    free(this -> buffer);
    free(this -> servaddr);
}

// Sends a message back to the client
void Manager::send_msg(std::string msg) 
{
    sendto(*(this -> sockfd), (const char *)msg.c_str(), strlen(msg.c_str()),
        MSG_CONFIRM, (const struct sockaddr *) this -> servaddr, 
            sizeof(*(this -> servaddr)));
}
