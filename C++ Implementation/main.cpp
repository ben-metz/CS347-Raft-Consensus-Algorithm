#include <unistd.h>
#include "manager.h"
#include <cstdlib>
#include <signal.h>

Manager *manager;

void signal_callback_handler(int signum) {
   std::cout << "Caught signal: " << signum << "\n";

   manager -> finish();

   // Terminate program
   exit(signum);
}

int main(){
    manager = (Manager*) malloc(sizeof(Manager));
    *manager = Manager();
    manager -> initialise(5);

    signal(SIGINT, signal_callback_handler);

    while(true);
}

