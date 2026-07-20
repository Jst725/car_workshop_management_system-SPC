#pragma once
#include <string>
using namespace std;

void addRecord(string name[], string plate[], string phone[], int& totalRecords, int SIZE);
void viewRecords(string name[], string plate[], string phone[], int& totalRecords);
void searchRecord(string name[], string plate[], string phone[], int totalRecords);
void customerManagementMenu(string name[], string plate[], string phone[], int& totalRecords, int SIZE);