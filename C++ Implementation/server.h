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
#include <atomic>

class Database;

class Manager;

class Server {
    private:
        int id;
        void server_function(std::atomic<bool>& running);
        void initThread(std::atomic<bool>& running);
        Manager* manager;

    public:
        std::thread* thread;
        Database *database;
        Server();
        int getID();
        void join();
        void diagnostic();
        void initialise(int id, Manager* manager, std::atomic<bool>& running, unsigned long long next_time, int delay);
        unsigned long long* next_time;
        int* delay; // Delay between update messages
};