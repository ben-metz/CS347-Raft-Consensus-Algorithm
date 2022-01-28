#include "server.h"
#include "manager.h"
#include "database.h"

std::mutex mtx;

// Server initialiser
Server::Server(){}

// Initialise the variables used by the server
void Server::initialise(int id, Manager* manager, std::atomic<bool>& running, 
    unsigned long long next_time, int delay, int port, int server_socket_address_count){
    this -> id = id;
    this -> database = (Database*) malloc(sizeof(Database));
    *this -> database = Database(DATABASE_SIZE);
    this -> manager = manager;
    this -> delay = (int*) malloc(sizeof(int));
    this -> next_time = (unsigned long long*) malloc(sizeof(unsigned long long));

    this -> server_address_added = (int*) malloc(sizeof(int));
    *this -> server_address_added = 0;

    *this -> delay = delay;
    *this -> next_time = next_time;

    this -> socket_addr = (struct server_socket_address*) malloc(sizeof(struct server_socket_address));
    this -> neighbours = (struct server_socket_address**) malloc(sizeof(struct server_socket_address*) * server_socket_address_count);

    this -> initSocket(port);

    this -> initThread(running);
}

// Function to be performed by the server
// This thread will take messages sent by other servers,
// feed them into the algorithm, and reply/send to other servers
void Server::server_function(std::atomic<bool>& running){
    using namespace std::chrono;

    int flags = fcntl(*this -> socket_addr -> fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(*this -> socket_addr -> fd, F_SETFL, flags);

    while(running){
        // Send details every so often
        unsigned long long ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        if (ms >= *this -> next_time){
            *this -> next_time = ms + *this -> delay;
            
            this -> send_details();
        }

        // Print received messages to console
        *this -> rcv_n = recvfrom(*this -> socket_addr -> fd, (char *) this -> rcv_buffer, 1024, 
            MSG_WAITALL, ( struct sockaddr *) &this -> socket_addr -> addr,
            this -> rcv_socklen);

        if (*this -> rcv_n != -1){
            this -> rcv_buffer[*this -> rcv_n] = '\0';

            std::cout << "Server " << this -> getID() << " Received Message: " << this -> rcv_buffer << '\n';

            this -> handleMessage(rcv_buffer);
        }
    }
}

// Handles messages received on the receive socket
void Server::handleMessage(char* msg){
    char* token = strtok(msg, " ");
    if (*token == 'U'){
        token = strtok(NULL, " "); // Get rid of server ID

        int* update_properties = (int*) malloc(sizeof(int) * 2);
        int i = 0;
        while (token = strtok(NULL, " ")){
            update_properties[i] = atoi(token);
            i++;
        }
        
        this -> database -> set_value(update_properties[0], update_properties[1]);

        free(update_properties);
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
    struct sockaddr_in rcv_addr;

    this -> rcv_buffer = (char*) malloc(sizeof(char) * 1024);
    this -> rcv_n = (int*) malloc(sizeof(int));
    this -> rcv_socklen = (socklen_t*) malloc(sizeof(socklen_t));
    *this -> rcv_socklen = sizeof(rcv_addr);
    int* receive_socket_fd = (int*) malloc(sizeof(int));

    // Creating socket file descriptor
    if ( (*receive_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("Receive socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    rcv_addr.sin_family = AF_INET;
    rcv_addr.sin_port = htons(port);
    rcv_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket with the server address
    if ( bind(*receive_socket_fd, 
            (const struct sockaddr *) &rcv_addr, 
            sizeof(rcv_addr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    this -> socket_addr -> server_socket_address_id = this -> getID();
    this -> socket_addr -> fd = receive_socket_fd;
    this -> socket_addr -> addr = rcv_addr;
}

// Adds this socket to the server_socket_addresss of the other servers
void Server::addToNeighbours(){
    this -> manager -> addSocket(this -> socket_addr);
}

// Return the ID of server
int Server::getID(){
    return this->id;
}

// Join the thread and close the receive socket
void Server::join(){
    this -> thread -> join();

    std::cout << "Closing Server " << this -> getID() << " Socket and Freeing Memory..." << "\n";
    close(*(this -> socket_addr -> fd));

    free(this -> socket_addr -> fd);

    free(this -> rcv_buffer);
    free(this -> rcv_n);
    free(this -> rcv_socklen);
}

// Adds a socket to the server_socket_addresss array of the server
void Server::addSocket(struct server_socket_address* addr){
    this -> neighbours[*this -> server_address_added] = addr;

    *this -> server_address_added += 1;

    std::string msg = "hello server " + std::to_string(addr -> server_socket_address_id) + " from " + std::to_string(this -> getID());

    this -> sendToServer(addr -> server_socket_address_id, msg);
}  

// Sends message to the server with the specified ID
void Server::sendToServer(int id, std::string msg){
    for (int i = 0; i < *this -> server_address_added; i++){
        if (this -> neighbours[i] -> server_socket_address_id == id){
            sendto(*(this -> neighbours[i] -> fd), (const char *) msg.c_str(), strlen(msg.c_str()),
                MSG_CONFIRM, (const struct sockaddr *) &(this -> neighbours[i] -> addr), 
                    sizeof(this -> neighbours[i] -> addr));
        }
    }
}

// Returns the servers receiving socket in a server_socket_address struct
struct server_socket_address* Server::getSocket(){
    return this -> socket_addr;
}

