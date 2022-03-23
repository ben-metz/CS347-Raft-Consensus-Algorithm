#include "database.h"

Database::Database(int size)
    : size(size)
    , data(nullptr)
{
    // Allocate an array on integers of `size` length
    this->data = (int *)calloc(size, sizeof(int));

    // Set all of the allocated integers to 0.
    for (int i = 0; i < size; i++)
    {
        this->data[i] = 0;
    }
}

Database::~Database()
{
    // Free the memory allocated in the constructor
    free(this->data);
    this->data = nullptr;
}

void Database::validateIndex(int index)
{
    // Assert that array indeces must be at least zero
    if (index < 0)
    {
        throw std::invalid_argument("Array index must be greater than zero.");
    }

    // Assert that array indeces must be strictly less than the size of the array
    if (index >= this->size)
    {
        throw std::invalid_argument("Array index must be less than the maximum data array size.");
    }
}

void Database::setValue(int index, int value)
{
    // Ensure the index is valid then update the data at that index.
    this->validateIndex(index);
    this->data[index] = value;
}

int *Database::getData()
{
    return this->data;
}

int Database::getSize()
{
    return this->size;
}

int Database::getValue(int index)
{
    // Ensure the index is valid then return the data at that index.
    this->validateIndex(index);
    return this->data[index];
}

bool Database::verify(int index, int value)
{
    // Ensure the index is valid then compare the stored data to an expected value
    this->validateIndex(index);
    return this->data[index] == value;
}