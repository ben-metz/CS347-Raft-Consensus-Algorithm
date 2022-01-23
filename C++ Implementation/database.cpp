#include "database.h"
#include <algorithm>
#include <iostream>

Database::Database(){
  this -> data = (int*) malloc(sizeof(int) * ARR_SIZE);
  this -> data[0] = 0;
  this -> data[1] = 0;
  this -> data[2] = 0;
  this -> data[3] = 0;
  this -> data[4] = 0;
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

int Database::get_value(int index) {
  this->validate_index(index);
  return this->data[index];
}