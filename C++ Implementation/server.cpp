#include "server.h"
#include "manager.h"
#include "database.h"
#include <atomic>
#include <chrono>

std::mutex mtx;

// Server initialiser
Server::Server(){}

// Function to be performed by the server
void Server::server_function(std::atomic<bool>& running){
    using namespace std::chrono;

    while(running){
        unsigned long long ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        if (ms >= *this -> next_time){
            *this -> next_time = ms + *this -> delay;
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
}

// Initialise the variables used by the server
void Server::initialise(int id, Manager* manager, std::atomic<bool>& running, unsigned long long next_time, int delay){
    this -> id = id;
    this -> database = (Database*) malloc(sizeof(Database));
    *this -> database = Database(5);
    this -> manager = manager;
    this -> delay = (int*) malloc(sizeof(int));
    this -> next_time = (unsigned long long*) malloc(sizeof(unsigned long long));

    *this -> delay = delay;
    *this -> next_time = next_time;

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

