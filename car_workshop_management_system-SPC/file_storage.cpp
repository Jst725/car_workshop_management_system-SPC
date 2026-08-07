#include "file_storage.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

using namespace std;

static Customer parseCustomerLine(const string& line, ifstream& inFile) {
    stringstream ss(line);
    Customer c;
    string vehicleCountStr;

    getline(ss, c.id, '|');
    getline(ss, c.name, '|');
    getline(ss, c.phone, '|');
    getline(ss, c.membershipTier, '|');
    getline(ss, vehicleCountStr, '|');

    int vehicleCount = 0;
    try { vehicleCount = stoi(vehicleCountStr); }
    catch (...) { vehicleCount = 0; }

    string vLine;
    for (int i = 0; i < vehicleCount && getline(inFile, vLine); i++) {
        stringstream vs(vLine);
        Vehicle v;
        string costStr;

        getline(vs, v.licensePlate, '|');
        getline(vs, v.brand, '|');
        getline(vs, v.model, '|');
        getline(vs, v.lastServiceDate, '|');
        getline(vs, v.status, '|');
        getline(vs, v.assignedTechnician, '|');
        getline(vs, costStr, '|');

        try { v.currentServiceCost = stod(costStr); }
        catch (...) { v.currentServiceCost = 0.0; }

        c.vehicles.push_back(v);
    }

    return c;
}


void saveCustomersToFile(const vector<Customer>& customers) {
    ofstream outFile("customers.txt");
    if (!outFile.is_open()) {
        cout << "[ERROR] Could not open customers.txt for writing!\n";
        return;
    }

    for (const auto& c : customers) {
        outFile << c.id << "|" << c.name << "|" << c.phone << "|"
            << c.membershipTier << "|" << c.vehicles.size() << "\n";

        for (const auto& v : c.vehicles) {
            outFile << v.licensePlate << "|" << v.brand << "|" << v.model << "|"
                << v.lastServiceDate << "|" << v.status << "|"
                << v.assignedTechnician << "|" << v.currentServiceCost << "\n";
        }
    }

    outFile.close();
}

void loadCustomersFromFile(vector<Customer>& customers) {
    ifstream inFile("customers.txt");
    if (!inFile.is_open()) {
        return; 
    }

    customers.clear();

    string firstLine;
    if (!getline(inFile, firstLine)) {
        inFile.close();
        return; 
    }

    bool isOldFormat = !firstLine.empty() &&
        all_of(firstLine.begin(), firstLine.end(), [](unsigned char ch) { return isdigit(ch); });

    if (isOldFormat) {
        int oldCount = 0;
        try { oldCount = stoi(firstLine); }
        catch (...) { oldCount = 0; }

        for (int i = 0; i < oldCount; i++) {
            string phone, name, plate;
            if (!getline(inFile, phone)) break;
            if (!getline(inFile, name)) break;
            if (!getline(inFile, plate)) break;

            Customer c;
            c.id = "CUS" + to_string(customers.size() + 101);
            c.name = name;
            c.phone = phone;
            c.membershipTier = "Standard";

            if (!plate.empty()) {
                Vehicle v;
                v.licensePlate = plate;
                v.brand = "Unknown";
                v.model = "Unknown";
                v.lastServiceDate = "N/A";
                v.status = "Awaiting Parts";
                v.assignedTechnician = "Unassigned";
                v.currentServiceCost = 0.0;
                c.vehicles.push_back(v);
            }

            customers.push_back(c);
        }
        inFile.close();

        cout << "[SYSTEM] Detected legacy customers.txt format - migrated "
            << customers.size() << " old record(s).\n";

        saveCustomersToFile(customers);
        return;
    }

    customers.push_back(parseCustomerLine(firstLine, inFile));

    string line;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        customers.push_back(parseCustomerLine(line, inFile));
    }

    inFile.close();
    cout << "[SYSTEM] Loaded " << customers.size() << " customer record(s) from customers.txt\n";
}


void saveAppointmentsToFile(const vector<Appointment>& appointments) {
    //Appointment module
}

void saveInvoicesToFile(const vector<Invoice>& invoices) {
    //Billing module
}

void saveAllData(const vector<Customer>& customers,
    const vector<Appointment>& appointments,
    const vector<Invoice>& invoices) {
    saveCustomersToFile(customers);
    saveAppointmentsToFile(appointments);
    saveInvoicesToFile(invoices);
}