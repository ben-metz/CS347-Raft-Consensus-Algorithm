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
#include <mutex>

#include "server.h"

#define IP          "127.0.0.1" // Loopback
#define PORT        12345
#define MAXLINE     1024 // Socket buffer size

// Socket details
int sockfd;
char buffer[MAXLINE];
struct sockaddr_in     servaddr;

// Worker threads (servers)
Server* servers;

std::mutex buffer_mutex;

// Sends a message back to the client
void send_msg(std::string msg) 
{
    sendto(sockfd, (const char *)msg.c_str(), strlen(msg.c_str()),
        MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
            sizeof(servaddr));
    printf("Hello message sent.\n");
}

// Function executed by the thread
void thread_server(){
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    std::string idstr = ss.str();
    std::string hello = "Hello from thread with ID: ";
    send_msg(hello + idstr);
}

// Initialises the socket used to communicate with the python client
void init_socket(){
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
   
    memset(&servaddr, 0, sizeof(servaddr));
       
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;
}
 
int main() 
{
  init_socket();

  servers = (Server*) malloc(sizeof(Server) * 5);
  
  // Initialise the server threads
  for(int i = 0; i < 5; i++){
      servers[i] = Server(i, &buffer_mutex); 
  }

  for(int i = 0; i < 5; i++){
      servers[i].join();
  }

  free(servers);

  std::cout << "foo and bar completed.\n";

  close(sockfd);

  return 0;
}