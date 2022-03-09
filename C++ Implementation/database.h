#include <stdexcept>
#include <iostream>

class Database
{
private:
    int *data;
    int size;
    void validate_index(int index);
public:
    Database(int size);
    int get_value(int index);
    int get_size();
    int *get_data();
    void set_value(int index, int value);
    bool verify(int index, int value);
};