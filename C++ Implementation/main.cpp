#include <unistd.h>
#include "manager.h"
#include <cstdlib>
#include <signal.h>

Manager *manager;

void signal_callback_handler(int signum) {
   std::cout << "\nCaught Signal: " << signum << "\n";

   delete manager;
   manager = nullptr;

   // Terminate program
   exit(signum);
}

int main(){
   manager = new Manager();
   manager -> initialise();

   signal(SIGINT, signal_callback_handler);

   while(true);
}

