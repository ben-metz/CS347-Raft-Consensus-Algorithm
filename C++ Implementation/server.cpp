#include "server.h"
#include "manager.h"
#include "database.h"

std::mutex mtx;

std::mutex socket_mutex[SERVER_COUNT];

extern std::atomic_bool running_;

// Server initialiser
Server::Server(){}

// Initialise the variables used by the server
void Server::initialise(int id, Manager* manager, 
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

    this -> raft = (Raft_Node*) malloc(sizeof(Raft_Node));
    *this -> raft = Raft_Node();

    raft_response = (char*) malloc(sizeof(char) * 250);

    this -> initSocket(port);

    this -> initThread();
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

    free(this -> neighbours);
    free(this -> socket_addr);
    free(this -> next_time);
    free(this -> server_address_added);

    free(this -> delay);
    free(this -> thread);

    free(this -> database -> get_size());
    free(this -> database -> get_data());
    free(this -> database);

    free(raft);
    free(raft_response);
}

// Function to be performed by the server
// This thread will take messages sent by other servers,
// feed them into the algorithm, and reply/send to other servers
void Server::server_function(){
    using namespace std::chrono;

    // int flags = fcntl(*this -> socket_addr -> fd, F_GETFL);
    // flags |= O_NONBLOCK;
    // fcntl(*this -> socket_addr -> fd, F_SETFL, flags);

    while(running_){
        // Send details every so often
        // unsigned long long ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        // if (ms >= *this -> next_time){
        //     *this -> next_time = ms + *this -> delay;
            
        //     this -> send_details();
        // }

        // Print received messages to console
        *this -> rcv_n = recvfrom(*this -> socket_addr -> fd, (char *) this -> rcv_buffer, 1024, 
            MSG_WAITALL, ( struct sockaddr *) &this -> socket_addr -> addr,
            this -> rcv_socklen);

        if (*this -> rcv_n != -1){
            this -> rcv_buffer[*this -> rcv_n] = '\0';

            std::cout << "Server " << this -> getID() << " Received Message: " << this -> rcv_buffer << '\n';

            this -> handleMessage(rcv_buffer);

            this -> send_details();
        }
    }
}

// Handles messages received on the receive socket
void Server::handleMessage(char* msg){
    raft -> input_message(msg,this -> raft_response);

    // Would send contents of raft response to appropriate server

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

    for (int i = 0; i < *database -> get_size(); i++){
        ss << this -> database -> get_value(i) << ' ';
    }

    mtx.lock();
    this -> manager -> send_msg(ss.str());
    mtx.unlock();
}

// Initialise the thread with the thread function
void Server::initThread(){
    this -> thread = (std::thread*) malloc(sizeof(std::thread));
    *this -> thread = std::thread(&Server::server_function, this); 
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

// Adds a socket to the server_socket_addresss array of the server
void Server::addSocket(struct server_socket_address* addr){
    this -> neighbours[*this -> server_address_added] = addr;

    *this -> server_address_added += 1;

    std::string msg = "Comms check: " + std::to_string(this -> getID()) + " -> " + std::to_string(addr -> server_socket_address_id);

    this -> sendToServer(addr -> server_socket_address_id, msg);
}  

// Sends message to the server with the specified ID
void Server::sendToServer(int id, std::string msg){
    int sock_id = this -> getSocketIndex(id);
    
    while(socket_mutex[id].try_lock());

    sendto(*(this -> neighbours[sock_id] -> fd), (const char *) msg.c_str(), strlen(msg.c_str()),
        MSG_CONFIRM, (const struct sockaddr *) &(this -> neighbours[sock_id] -> addr), 
            sizeof(this -> neighbours[sock_id] -> addr));

    socket_mutex[id].unlock();
}

// Get index of socket in neighbours for server with passed ID
int Server::getSocketIndex(int server_id){
    if (server_id > this -> getID()){
        return server_id - 1;
    } else {
        return server_id;
    }
}

// Returns the servers receiving socket in a server_socket_address struct
struct server_socket_address* Server::getSocket(){
    return this -> socket_addr;
}
