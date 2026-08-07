#pragma once
#include <string>
using namespace std;

// File Handling Functions
void saveCustomers(const string name[], const string plate[], const string phone[], int totalRecords);
void loadCustomers(string name[], string plate[], string phone[], int& totalRecords);

// Customer Management Functions
void addRecord(string name[], string plate[], string phone[], int& totalRecords, int SIZE);
void viewRecords(string name[], string plate[], string phone[], int& totalRecords);
void searchRecord(string name[], string plate[], string phone[], int totalRecords);
void customerManagementMenu(string name[], string plate[], string phone[], int& totalRecords, int SIZE);