#include <unistd.h>
#include "manager.h"
#include <cstdlib>
#include <signal.h>

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
    manager -> initialise(10);

    signal(SIGINT, signal_callback_handler);

    while(true);
}

