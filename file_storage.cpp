#include "file_storage.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

using namespace std;

// Splits a line by '|' into its raw fields (does not consume the stream).
static vector<string> splitFields(const string& line) {
    vector<string> fields;
    stringstream ss(line);
    string field;
    while (getline(ss, field, '|')) {
        fields.push_back(field);
    }
    return fields;
}

static double toDoubleSafe(const string& s) {
    try { return stod(s); } catch (...) { return 0.0; }
}
static int toIntSafe(const string& s) {
    try { return stoi(s); } catch (...) { return 0; }
}

// ===================================================
// CUSTOMER + VEHICLE FILE I/O (my part)
//
// customers.txt : phone|name|membershipTier|totalPurchases
// vehicles.txt  : phone|plate|brand|model|lastServiceDate|status|assignedTechnician|
//                 partsCost|laborCost|lastServiceDescription|lastPartsUsed|currentServiceCost
// ===================================================

void saveCustomersToFile(const vector<Customer>& customers) {
    ofstream custFile("customers.txt");
    if (!custFile.is_open()) {
        cout << "[ERROR] Could not open customers.txt for writing!\n";
        return;
    }
    for (const auto& c : customers) {
        custFile << c.phone << "|" << c.name << "|" << c.membershipTier << "|"
                  << c.totalPurchases << "\n";
    }
    custFile.close();

    ofstream vehFile("vehicles.txt");
    if (!vehFile.is_open()) {
        cout << "[ERROR] Could not open vehicles.txt for writing!\n";
        return;
    }
    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            vehFile << c.phone << "|" << v.licensePlate << "|" << v.brand << "|" << v.model << "|"
                    << v.lastServiceDate << "|" << v.status << "|" << v.assignedTechnician << "|"
                    << v.partsCost << "|" << v.laborCost << "|"
                    << v.lastServiceDescription << "|" << v.lastPartsUsed << "|"
                    << v.currentServiceCost << "\n";
        }
    }
    vehFile.close();
}

void loadCustomersFromFile(vector<Customer>& customers) {
    ifstream custFile("customers.txt");
    if (!custFile.is_open()) {
        return; // First run - no data yet, that's fine
    }

    customers.clear();
    bool legacyDetected = false;
    string line;

    while (getline(custFile, line)) {
        if (line.empty()) continue;
        vector<string> fields = splitFields(line);

        if (fields.size() >= 5) {
            // ---- OLDEST legacy format: id|name|phone|tier|vehicleCount (with
            //      vehicles embedded directly below the customer line) ----
            legacyDetected = true;

            Customer c;
            // fields[0] = old id -> discarded, phone is now the only identifier
            c.name = fields[1];
            c.phone = fields[2];
            c.membershipTier = fields[3];
            c.totalPurchases = 0.0;

            int vehicleCount = toIntSafe(fields[4]);
            string vLine;
            for (int i = 0; i < vehicleCount && getline(custFile, vLine); i++) {
                vector<string> vf = splitFields(vLine);
                Vehicle v;
                if (vf.size() >= 7) {
                    v.licensePlate = vf[0];
                    v.brand = vf[1];
                    v.model = vf[2];
                    v.lastServiceDate = vf[3];
                    v.status = vf[4];
                    v.assignedTechnician = vf[5];
                    v.currentServiceCost = toDoubleSafe(vf[6]);
                }
                c.vehicles.push_back(v);
            }
            customers.push_back(c);
        }
        else {
            // ---- phone|name|tier[|totalPurchases] ----
            Customer c;
            c.phone = fields.size() > 0 ? fields[0] : "";
            c.name = fields.size() > 1 ? fields[1] : "";
            c.membershipTier = fields.size() > 2 ? fields[2] : "Regular";
            c.totalPurchases = fields.size() > 3 ? toDoubleSafe(fields[3]) : 0.0;
            customers.push_back(c);
        }
    }
    custFile.close();

    if (legacyDetected) {
        cout << "[SYSTEM] Detected legacy customer file (with Customer ID) - migrated "
             << customers.size() << " record(s) to the current phone-based format.\n";
        saveCustomersToFile(customers); // rewrite both files in the current format
        return; // vehicles were already captured above from the embedded legacy data
    }

    // ---- Load vehicles.txt and re-attach each vehicle to its owner by phone ----
    ifstream vehFile("vehicles.txt");
    if (!vehFile.is_open()) {
        cout << "[SYSTEM] Loaded " << customers.size() << " customer record(s). (No vehicles.txt found yet.)\n";
        return;
    }

    int vehiclesLoaded = 0;
    while (getline(vehFile, line)) {
        if (line.empty()) continue;
        vector<string> vf = splitFields(line);
        if (vf.size() < 8) continue; // malformed line, skip

        const string& ownerPhone = vf[0];
        Vehicle v;
        v.licensePlate = vf[1];
        v.brand = vf[2];
        v.model = vf[3];
        v.lastServiceDate = vf[4];
        v.status = vf[5];
        v.assignedTechnician = vf[6];

        if (vf.size() >= 12) {
            // current format: ...|partsCost|laborCost|lastServiceDescription|lastPartsUsed|currentServiceCost
            v.partsCost = toDoubleSafe(vf[7]);
            v.laborCost = toDoubleSafe(vf[8]);
            v.lastServiceDescription = vf[9];
            v.lastPartsUsed = vf[10];
            v.currentServiceCost = toDoubleSafe(vf[11]);
        }
        else {
            // previous format: ...|currentServiceCost (no parts/labor split yet)
            v.currentServiceCost = toDoubleSafe(vf[7]);
        }

        for (auto& c : customers) {
            if (c.phone == ownerPhone) {
                c.vehicles.push_back(v);
                vehiclesLoaded++;
                break;
            }
        }
    }
    vehFile.close();

    cout << "[SYSTEM] Loaded " << customers.size() << " customer record(s) and "
         << vehiclesLoaded << " vehicle record(s).\n";
}

// ===================================================
// APPOINTMENT FILE I/O
// (best-effort implementation based on the appointments.txt format that was
//  already in the project - Appointment module owner should double check this)
//
// appointments.txt : appointmentId|customerId|licensePlate|date|time|serviceType|isActive(0/1)
// ===================================================

void saveAppointmentsToFile(const vector<Appointment>& appointments) {
    ofstream outFile("appointments.txt");
    if (!outFile.is_open()) {
        cout << "[ERROR] Could not open appointments.txt for writing!\n";
        return;
    }
    for (const auto& a : appointments) {
        outFile << a.appointmentId << "|" << a.customerId << "|" << a.licensePlate << "|"
                << a.date << "|" << a.time << "|" << a.serviceType << "|"
                << (a.isActive ? 1 : 0) << "\n";
    }
    outFile.close();
}

void loadAppointmentsFromFile(vector<Appointment>& appointments) {
    ifstream inFile("appointments.txt");
    if (!inFile.is_open()) return;

    appointments.clear();
    string line;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        vector<string> f = splitFields(line);
        if (f.size() < 7) continue; // malformed line, skip

        Appointment a;
        a.appointmentId = f[0];
        a.customerId = f[1];
        a.licensePlate = f[2];
        a.date = f[3];
        a.time = f[4];
        a.serviceType = f[5];
        a.isActive = (toIntSafe(f[6]) != 0);
        appointments.push_back(a);
    }
    inFile.close();
}

// ===================================================
// INVOICE FILE I/O
// PLACEHOLDER: Invoice now contains a nested vector<InvoiceItem>, which needs
// its own serialization design. Billing module owner should implement this.
// ===================================================

void saveInvoicesToFile(const vector<Invoice>& invoices) {
    // TODO: Billing module owner should implement this (Invoice.items is nested data).
}

void loadInvoicesFromFile(vector<Invoice>& invoices) {
    // TODO: Billing module owner should implement this (Invoice.items is nested data).
}

// ===================================================
// COMBINED HELPERS (used by main.cpp)
// ===================================================

void loadAllData(vector<Customer>& customers, vector<Appointment>& appointments, vector<Invoice>& invoices) {
    loadCustomersFromFile(customers);
    loadAppointmentsFromFile(appointments);
    loadInvoicesFromFile(invoices);
}

void saveAllData(const vector<Customer>& customers,
                  const vector<Appointment>& appointments,
                  const vector<Invoice>& invoices) {
    saveCustomersToFile(customers);
    saveAppointmentsToFile(appointments);
    saveInvoicesToFile(invoices);
}
