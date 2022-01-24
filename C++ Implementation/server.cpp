#include "server.h"
#include "manager.h"
#include "database.h"
#include <atomic>

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
void Server::server_function(std::atomic<bool>& running){
    while(running){
    //for (int i = 0; i < 5; i++){
        //this -> diagnostic();

        std::ostringstream ss;

        ss << this -> getID() << ":";

        for (int i = 0; i < database -> get_size(); i++){
            ss << this -> database -> get_value(i) << ' ';
        }

        mtx.lock();
        this -> manager -> send_msg(ss.str());
        mtx.unlock();
    }
}

// Server initialiser
Server::Server(){}

// Initialise the variables used by the server
void Server::initialise(int id, Manager* manager, std::atomic<bool>& running){
    this -> id = id;
    this -> database = (Database*) malloc(sizeof(Database));
    *this -> database = Database(5);
    this -> manager = manager;

    //this -> diagnostic();

    this -> initThread(running);
}

// Initialise the thread with the thread function
void Server::initThread(std::atomic<bool>& running){
    this -> thread = (std::thread*) malloc(sizeof(std::thread));
    *this -> thread = std::thread(&Server::server_function, this, std::ref(running)); 
}

// Return the ID of server
int Server::getID(){
    return this->id;
}

// Join the thread
void Server::join(){
    this -> thread -> join();
}

