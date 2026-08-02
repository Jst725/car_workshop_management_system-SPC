#include "file_storage.h"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

// Loads existing records from 'customers.txt' into parallel arrays upon startup
void loadCustomers(string name[], string plate[], string phone[], int& totalRecords, int SIZE) {
    ifstream inFile("customers.txt");
    if (!inFile.is_open()) {
        return; // File does not exist yet (first time running)
    }

    totalRecords = 0;
    string line;
    while (getline(inFile, line) && totalRecords < SIZE) {
        if (line.empty()) continue;

        stringstream ss(line);
        string filePhone, fileName, filePlate;

        // Reads Pipe-delimited (|) lines: Phone|Name|Plate
        if (getline(ss, filePhone, '|') &&
            getline(ss, fileName, '|') &&
            getline(ss, filePlate, '|')) {

            phone[totalRecords] = filePhone;
            name[totalRecords] = fileName;
            plate[totalRecords] = filePlate;
            totalRecords++;
        }
    }
    inFile.close();
}

// Writes current active array records to 'customers.txt'
void saveCustomers(const string name[], const string plate[], const string phone[], int totalRecords) {
    ofstream outFile("customers.txt");
    if (!outFile.is_open()) {
        cout << "[ERROR] Could not open file to save customer data!\n";
        return;
    }

    for (int i = 0; i < totalRecords; i++) {
        outFile << phone[i] << "|" << name[i] << "|" << plate[i] << "\n";
    }

    outFile.close();
}