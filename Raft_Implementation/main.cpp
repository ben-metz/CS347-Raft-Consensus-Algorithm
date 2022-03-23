#include <unistd.h>
#include "manager.h"
#include <cstdlib>
#include <signal.h>

Manager *manager;

void signalCallbackHandler(int signum) {
   std::cout << "\nCaught Signal: " << signum << "\n";

   // Clean up the allocated manager
   if (manager)
   {
      delete manager;
      manager = nullptr;
   }

   // Report a successful shut down
   printf("Successfully shut down\n");

   // Terminate program
   exit(signum);
}

int main(){
   // Register our signal handler
   signal(SIGINT, signalCallbackHandler);

   // Allocate a new manager object
   manager = new Manager();

   // Wait indefinitely
   while(true);
}

