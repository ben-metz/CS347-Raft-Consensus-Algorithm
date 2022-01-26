#include "server.h"
#include "manager.h"
#include "database.h"
#include <atomic>
#include <chrono>
#include <fcntl.h>

std::mutex mtx;

// Server initialiser
Server::Server(){}

// Initialise the variables used by the server
void Server::initialise(int id, Manager* manager, std::atomic<bool>& running, unsigned long long next_time, int delay, int port){
    this -> id = id;
    this -> database = (Database*) malloc(sizeof(Database));
    *this -> database = Database(5);
    this -> manager = manager;
    this -> delay = (int*) malloc(sizeof(int));
    this -> next_time = (unsigned long long*) malloc(sizeof(unsigned long long));

    *this -> delay = delay;
    *this -> next_time = next_time;

    this -> initThread(running);

    this -> initSocket(port);
}

// Function to be performed by the server
// This thread will take messages sent by other servers,
// feed them into the algorithm, and reply/send to other servers
void Server::server_function(std::atomic<bool>& running){
    using namespace std::chrono;

    int flags = fcntl(*receive_socket_fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(*receive_socket_fd, F_SETFL, flags);

    while(running){
        unsigned long long ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        if (ms >= *this -> next_time){
            *this -> next_time = ms + *this -> delay;
            
            this -> send_details();
        }

        *this -> rcv_n = recvfrom(*receive_socket_fd, (char *) this -> rcv_buffer, 1024, 
            MSG_WAITALL, ( struct sockaddr *) &rcv_addr,
            this -> rcv_socklen);

        if (*this -> rcv_n != -1){
            this -> rcv_buffer[*this -> rcv_n] = '\0';

            std::cout << "Server " << this -> getID() << " Received " << this -> rcv_buffer << '\n';
        }
    }
}

// Function to send details to python client
void Server::send_details(){
    std::ostringstream ss;

    ss << this -> getID() << ":";

    for (int i = 0; i < database -> get_size(); i++){
        ss << this -> database -> get_value(i) << ' ';
    }

    mtx.lock();
    this -> manager -> send_msg(ss.str());
    mtx.unlock();
}

// Initialise the thread with the thread function
void Server::initThread(std::atomic<bool>& running){
    this -> thread = (std::thread*) malloc(sizeof(std::thread));
    *this -> thread = std::thread(&Server::server_function, this, std::ref(running)); 
}

// Initialise the listening socket
void Server::initSocket(int port){
    this -> rcv_buffer = (char*) malloc(sizeof(char) * 1024);
    this -> rcv_n = (int*) malloc(sizeof(int));
    this -> rcv_socklen = (socklen_t*) malloc(sizeof(socklen_t));
    *this -> rcv_socklen = sizeof(this -> rcv_addr);
    this -> receive_socket_fd = (int*) malloc(sizeof(int));

    // Creating socket file descriptor
    if ( (*(this -> receive_socket_fd) = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("Receive socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    this -> rcv_addr.sin_family = AF_INET;
    this -> rcv_addr.sin_port = htons(port);
    this -> rcv_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket with the server address
    if ( bind(*receive_socket_fd, 
            (const struct sockaddr *) &rcv_addr, 
            sizeof(rcv_addr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

// Return the ID of server
int Server::getID(){
    return this->id;
}

// Join the thread and close the receive socket
void Server::join(){
    this -> thread -> join();

    std::cout << "Closing Server " << this -> getID() << " Socket and Freeing Memory..." << "\n";
    close(*(this -> receive_socket_fd));

    free(this -> receive_socket_fd);

    free(this -> rcv_buffer);
    free(this -> rcv_n);
    free(this -> rcv_socklen);
}

