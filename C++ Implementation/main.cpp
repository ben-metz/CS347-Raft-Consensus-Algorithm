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

    signal(SIGINT, signal_callback_handler);

    unsigned int microsecond = 1000000000;
    usleep(microsecond);

    manager -> send_msg("END");

    manager -> finish();
}

