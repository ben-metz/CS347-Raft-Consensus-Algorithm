#include <stdexcept>

#define ARR_SIZE 5

class Database {
  private:
    int *data;
    int size = ARR_SIZE;
    void validate_index(int index);

  public:
    Database();
    int get_value(int index);
    int get_size();
    int* get_data();
    void set_value(int index, int value);
};