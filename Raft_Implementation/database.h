#pragma once

#include <stdexcept>
#include <iostream>

class Database
{
private:
    int *data;
    int size;
    void validateIndex(int index);
public:
    Database(int size);
    ~Database();
    
    void setValue(int index, int value);
    int getValue(int index);
    int getSize();
    int *getData();
    bool verify(int index, int value);
};