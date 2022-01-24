#include "database.h"
#include <iostream>

Database::Database(){
  const int size = ARR_SIZE;

  this -> data = (int*) calloc(size, sizeof(int));

  for (int i = 0; i < size; i++) {
    this -> data[i] = 0;
  }
}

void Database::validate_index(int index) {
  if (index < 0) {
    throw std::invalid_argument("Array index must be greater than zero.");
  }
  if (index >= this->size) {
    throw std::invalid_argument("Array index must be less than the maximum data array size.");
  }
}

void Database::set_value(int index, int value) {
  this->validate_index(index);
  this->data[index] = value;
}

int* Database::get_data() {
  return this->data;
}

int Database::get_size() {
  return this->size;
}

int Database::get_value(int index) {
  this->validate_index(index);
  return this->data[index];
}