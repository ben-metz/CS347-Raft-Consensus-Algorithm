// thread example
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>

#include "server.h"

#include <sstream>

// Function to be performed by the server
void Server::server_function(){
    std::ostringstream ss;

    ss << std::this_thread::get_id();

    std::string idstr = "hello from " + ss.str();

    this -> send_to_client(idstr);
}

// Server initialiser
Server::Server(int id, int* sockfd, struct sockaddr_in* socket_address){
    this -> id = id;
    this -> database = (Database*) malloc(sizeof(Database));
    *this -> database = Database();
    this -> sockfd = sockfd;
    this -> msg_socket = socket_address;

    for (int i = 0; i < 5; i++){
        std::cout << database -> get_data()[i];
    }

    this -> thread = (std::thread*) malloc(sizeof(std::thread));
    *this -> thread = std::thread(&Server::server_function, this); 

    for (int i = 0; i < 5; i++){
        std::cout << database -> get_data()[i];
    }
}

// Return the ID of server
int Server::getID(){
    return this->id;
}

// Join the thread
void Server::join(){
    this -> thread -> join();
}

// Send a message to the client
void Server::send_to_client(std::string msg){
    sendto(*(this -> sockfd), (const char *)msg.c_str(), strlen(msg.c_str()),
        MSG_CONFIRM, (const struct sockaddr *) this -> msg_socket, 
            sizeof(*(this -> msg_socket)));

    
    std::cout << "\n";
    
    printf("Message sent from server.\n");
}

