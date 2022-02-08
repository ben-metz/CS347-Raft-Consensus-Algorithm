#include "raft.h"
#include <iostream>

Raft_Node::Raft_Node(){
    this -> state = 0;
}

void Raft_Node::input_message(char* msg, char* output_buffer){
    std::cout << "Raft Input: " << msg << '\n';

    int i = 0;
    while(msg[i] != '\0'){
        output_buffer[i] = msg[i];
        i+=1;
    }
    output_buffer[i] = '\0';

    std::cout << "Raft Response: " << output_buffer << "\n\n";
}