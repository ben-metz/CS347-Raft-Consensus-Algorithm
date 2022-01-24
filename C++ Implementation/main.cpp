#include "manager.h"
#include <unistd.h>

int main(){
    Manager *manager = (Manager*) malloc(sizeof(Manager));
    *manager = Manager();

    unsigned int microsecond = 100000;
    usleep(microsecond);//sleeps for 3 second

    manager -> send_msg("END");

    manager -> finish();
}