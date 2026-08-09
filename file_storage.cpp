#include "file_storage.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>

using namespace std;

// Static helper to parse a customer line and its associated vehicles & parts
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
        string partsCostStr, laborCostStr, currentCostStr, partCountStr;

        getline(vs, v.licensePlate, '|');
        getline(vs, v.brand, '|');
        getline(vs, v.model, '|');
        getline(vs, v.lastServiceDate, '|');
        getline(vs, v.status, '|');
        getline(vs, v.assignedTechnician, '|');

        getline(vs, partsCostStr, '|');
        getline(vs, laborCostStr, '|');
        getline(vs, currentCostStr, '|');
        getline(vs, partCountStr, '|');

        try { v.partsCost = stod(partsCostStr); }
        catch (...) { v.partsCost = 0.0; }
        try { v.laborCost = stod(laborCostStr); }
        catch (...) { v.laborCost = 0.0; }
        try { v.currentServiceCost = stod(currentCostStr); }
        catch (...) { v.currentServiceCost = 0.0; }

        int partCount = 0;
        try { partCount = stoi(partCountStr); }
        catch (...) { partCount = 0; }

        for (int p = 0; p < partCount; p++) {
            string partToken;
            if (getline(vs, partToken, '|')) {
                size_t delimPos = partToken.find(';');
                if (delimPos != string::npos) {
                    PartItem item;
                    item.name = partToken.substr(0, delimPos);
                    try { item.price = stod(partToken.substr(delimPos + 1)); }
                    catch (...) { item.price = 0.0; }
                    v.parts.push_back(item);

                    stringstream logSs;
                    logSs << item.name << " ($" << fixed << setprecision(2) << item.price << ")";

                    ServiceLog logEntry;
                    logEntry.date = v.lastServiceDate;
                    logEntry.description = logSs.str();
                    logEntry.cost = item.price;
                    v.activeServiceLogs.push_back(logEntry);
                }
            }
        }

        c.vehicles.push_back(v);
    }

    return c;
}

// ===================== CUSTOMER FILE OPERATIONS =====================

void loadCustomersFromFile(vector<Customer>& customers) {
    customers.clear();
    ifstream inFile("customers.txt");
    if (!inFile.is_open()) return;

    string line;
    while (getline(inFile, line)) {
        if (!line.empty()) {
            customers.push_back(parseCustomerLine(line, inFile));
        }
    }
    inFile.close();
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
                << v.assignedTechnician << "|"
                << v.partsCost << "|"
                << v.laborCost << "|"
                << v.currentServiceCost << "|"
                << v.parts.size();

            for (const auto& item : v.parts) {
                outFile << "|" << item.name << ";" << item.price;
            }
            outFile << "\n";
        }
    }

    outFile.close();
}

// ===================== APPOINTMENT FILE OPERATIONS =====================

void loadAppointmentsFromFile(vector<Appointment>& appointments) {
    appointments.clear();
    ifstream inFile("appointments.txt");
    if (!inFile.is_open()) return;

    string line;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        Appointment a;
        string activeStr;

        getline(ss, a.appointmentId, '|');
        getline(ss, a.customerId, '|');
        getline(ss, a.licensePlate, '|');
        getline(ss, a.date, '|');
        getline(ss, a.time, '|');
        getline(ss, a.serviceType, '|');
        getline(ss, activeStr, '|');

        a.isActive = (activeStr == "1" || activeStr == "true");
        appointments.push_back(a);
    }
    inFile.close();
}

void saveAppointmentsToFile(const vector<Appointment>& appointments) {
    ofstream outFile("appointments.txt");
    if (!outFile.is_open()) {
        cout << "[ERROR] Could not open appointments.txt for writing!\n";
        return;
    }

    for (const auto& a : appointments) {
        outFile << a.appointmentId << "|" << a.customerId << "|"
            << a.licensePlate << "|" << a.date << "|"
            << a.time << "|" << a.serviceType << "|"
            << (a.isActive ? "1" : "0") << "\n";
    }
    outFile.close();
}

// ===================== INVOICE FILE OPERATIONS =====================

void loadInvoicesFromFile(vector<Invoice>& invoices) {
    invoices.clear();
    ifstream inFile("invoices.txt");
    if (!inFile.is_open()) return;

    string line;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        Invoice inv;
        string partsStr, laborStr, discountStr, taxStr, totalStr, paidStr;

        getline(ss, inv.invoiceId, '|');
        getline(ss, inv.customerId, '|');
        getline(ss, inv.licensePlate, '|');
        getline(ss, inv.date, '|');
        getline(ss, partsStr, '|');
        getline(ss, laborStr, '|');
        getline(ss, discountStr, '|');
        getline(ss, taxStr, '|');
        getline(ss, totalStr, '|');
        getline(ss, inv.paymentType, '|');
        getline(ss, paidStr, '|');

        try { inv.partsCost = stod(partsStr); }
        catch (...) { inv.partsCost = 0.0; }
        try { inv.laborCost = stod(laborStr); }
        catch (...) { inv.laborCost = 0.0; }
        try { inv.discountAmount = stod(discountStr); }
        catch (...) { inv.discountAmount = 0.0; }
        try { inv.taxAmount = stod(taxStr); }
        catch (...) { inv.taxAmount = 0.0; }
        try { inv.totalAmount = stod(totalStr); }
        catch (...) { inv.totalAmount = 0.0; }
        inv.isPaid = (paidStr == "1" || paidStr == "true");

        invoices.push_back(inv);
    }
    inFile.close();
}

void saveInvoicesToFile(const vector<Invoice>& invoices) {
    ofstream outFile("invoices.txt");
    if (!outFile.is_open()) {
        cout << "[ERROR] Could not open invoices.txt for writing!\n";
        return;
    }

    for (const auto& inv : invoices) {
        outFile << inv.invoiceId << "|" << inv.customerId << "|"
            << inv.licensePlate << "|" << inv.date << "|"
            << inv.partsCost << "|" << inv.laborCost << "|"
            << inv.discountAmount << "|" << inv.taxAmount << "|"
            << inv.totalAmount << "|" << inv.paymentType << "|"
            << (inv.isPaid ? "1" : "0") << "\n";
    }
    outFile.close();
}

// ===================== BUNDLED MASTER LOAD / SAVE =====================

void loadAllData(vector<Customer>& customers, vector<Appointment>& appointments, vector<Invoice>& invoices) {
    loadCustomersFromFile(customers);
    loadAppointmentsFromFile(appointments);
    loadInvoicesFromFile(invoices);

    for (auto& c : customers) {
        c.totalPurchases = 0.0;
        for (const auto& inv : invoices) {
            if (inv.customerId == c.id && inv.isPaid) {
                c.totalPurchases += inv.totalAmount;
            }
        }
    }
}

void saveAllData(const vector<Customer>& customers, const vector<Appointment>& appointments, const vector<Invoice>& invoices) {
    saveCustomersToFile(customers);
    saveAppointmentsToFile(appointments);
    saveInvoicesToFile(invoices);
}