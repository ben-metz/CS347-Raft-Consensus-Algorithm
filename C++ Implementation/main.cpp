#include <unistd.h>
#include "manager.h"
#include <cstdlib>
#include <signal.h>

#define MESSAGES_PER_SECOND 100

Manager *manager;

void signal_callback_handler(int signum) {
   std::cout << "\nCaught Signal: " << signum << "\n";

   manager -> finish();

   // Terminate program
   exit(signum);
}

int main(){
    manager = (Manager*) malloc(sizeof(Manager));
    *manager = Manager();
    manager -> initialise(MESSAGES_PER_SECOND);

    signal(SIGINT, signal_callback_handler);

    while(true);
}

