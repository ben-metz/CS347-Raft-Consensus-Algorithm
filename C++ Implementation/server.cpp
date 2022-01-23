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

// Function to be performed by the server
void Server::server_function(){
    lock -> lock();
    std::cout << "hello\n";
    lock -> unlock();
}

// Server initialiser
Server::Server(int id, std::mutex *lock){
    this -> id = id;
    this -> thread = std::thread(&Server::server_function, this); 
    this -> database = Database();
    this -> lock = lock;
}

// Return the ID of server
int Server::getID(){
    return this->id;
}

// Join the thread
void Server::join(){
    this -> thread.join();
}

