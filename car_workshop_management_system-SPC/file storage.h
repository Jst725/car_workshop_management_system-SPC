#pragma once
#include <string>

// Function declarations to load and save customer array data
void loadCustomers(std::string name[], std::string plate[], std::string phone[], int& totalRecords, int SIZE);
void saveCustomers(const std::string name[], const std::string plate[], const std::string phone[], int totalRecords);