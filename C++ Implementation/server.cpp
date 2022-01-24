#include "server.h"
#include "manager.h"
#include "database.h"

std::mutex mtx;

void Server::diagnostic() {
    int size = this -> database -> get_size();

    std::cout << "\nSize: " << size << std::endl;

    std:: cout << "Data: ";
    for (int i = 0; i < size; i++){
        std::cout << this -> database -> get_value(i);
    }
    std::cout << std:: endl;
}

// Function to be performed by the server
void Server::server_function(){
    for (int i = 0; i < 3; i++){
        std::cout << "\nIn Thread Function \n";

        this -> diagnostic();

        std::ostringstream ss;

        ss << std::this_thread::get_id();

        std::string idstr = "hello from " + ss.str();

        mtx.lock();
        this -> manager -> send_msg(idstr);
        mtx.unlock();
    }
}

// Server initialiser
Server::Server(){}

void Server::initialise(int id, Manager* manager){
    this -> id = id;
    this -> database = (Database*) malloc(sizeof(Database));
    *this -> database = Database(5);
    this -> manager = manager;

    std::cout << "\nBefore Thread ";

    this -> diagnostic();

    this -> initThread();
}

// Initialise the thread with the thread function
void Server::initThread(){
    this -> thread = (std::thread*) malloc(sizeof(std::thread));
    *this -> thread = std::thread(&Server::server_function, this); 
}

// Return the ID of server
int Server::getID(){
    return this->id;
}

// Join the thread
void Server::join(){
    this -> thread -> join();
}

