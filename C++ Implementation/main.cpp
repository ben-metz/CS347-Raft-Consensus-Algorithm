#include <unistd.h>
#include "manager.h"
#include <cstdlib>
#include <signal.h>

#define MESSAGES_PER_SECOND 5

Manager *manager;

void signal_callback_handler(int signum) {
   std::cout << "\nCaught Signal: " << signum << "\n";

   if (manager)
   {
      delete manager;
      manager = nullptr;
   }

   // Not reached for some reason?
   // Maybe exiting thread is de-allocated before finishing?
   printf("Successfully shut down\n");

   // Terminate program
   exit(signum);
}

int main(){
   signal(SIGINT, signal_callback_handler);

   manager = new Manager(MESSAGES_PER_SECOND);

   while(true);
}

