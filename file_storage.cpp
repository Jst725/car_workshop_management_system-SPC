#include "file_storage.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

using namespace std;

// Splits a line by '|' into its raw fields.
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
    try { return stod(s); }
    catch (...) { return 0.0; }
}
static int toIntSafe(const string& s) {
    try { return stoi(s); }
    catch (...) { return 0; }
}

// ===================================================
// CUSTOMER + VEHICLE FILE I/O
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
            // Serialize itemized vector<PartItem>
            stringstream partsSs;
            for (size_t i = 0; i < v.parts.size(); ++i) {
                const auto& p = v.parts[i];
                partsSs << p.partId << "," << p.name << "," << p.quantity << ","
                    << p.unitPrice << "," << p.price << "," << (p.isFromAppointment ? 1 : 0);
                if (i + 1 < v.parts.size()) partsSs << ";";
            }

            vehFile << c.phone << "|" << v.licensePlate << "|" << v.brand << "|" << v.model << "|"
                << v.lastServiceDate << "|" << v.status << "|" << v.assignedTechnician << "|"
                << v.partsCost << "|" << v.laborCost << "|"
                << v.lastServiceDescription << "|" << v.lastPartsUsed << "|"
                << v.currentServiceCost << "|" << partsSs.str() << "\n";
        }
    }
    vehFile.close();
}

void loadCustomersFromFile(vector<Customer>& customers) {
    ifstream custFile("customers.txt");
    if (!custFile.is_open()) return;

    customers.clear();
    bool legacyDetected = false;
    string line;

    while (getline(custFile, line)) {
        if (line.empty()) continue;
        vector<string> fields = splitFields(line);

        if (fields.size() >= 5) {
            legacyDetected = true;
            Customer c;
            c.name = fields[1];
            c.phone = fields[2];
            c.membershipTier = fields[3];
            c.totalPurchases = 0.0;

            int vehicleCount = toIntSafe(fields[4]);
            string vLine;
            for (int i = 0; i < vehicleCount&& getline(custFile, vLine); i++) {
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
        cout << "[SYSTEM] Detected legacy customer file - migrated "
            << customers.size() << " record(s) to phone-based format.\n";
        saveCustomersToFile(customers);
        return;
    }

    ifstream vehFile("vehicles.txt");
    if (!vehFile.is_open()) {
        cout << "[SYSTEM] Loaded " << customers.size() << " customer record(s).\n";
        return;
    }

    int vehiclesLoaded = 0;
    while (getline(vehFile, line)) {
        if (line.empty()) continue;
        vector<string> vf = splitFields(line);
        if (vf.size() < 8) continue;

        const string& ownerPhone = vf[0];
        Vehicle v;
        v.licensePlate = vf[1];
        v.brand = vf[2];
        v.model = vf[3];
        v.lastServiceDate = vf[4];
        v.status = vf[5];
        v.assignedTechnician = vf[6];

        if (vf.size() >= 12) {
            v.partsCost = toDoubleSafe(vf[7]);
            v.laborCost = toDoubleSafe(vf[8]);
            v.lastServiceDescription = vf[9];
            v.lastPartsUsed = vf[10];
            v.currentServiceCost = toDoubleSafe(vf[11]);

            if (vf.size() >= 13 && !vf[12].empty()) {
                stringstream partsSs(vf[12]);
                string pToken;
                while (getline(partsSs, pToken, ';')) {
                    if (pToken.empty()) continue;
                    stringstream itemSs(pToken);
                    string itemField;
                    vector<string> pf;
                    while (getline(itemSs, itemField, ',')) {
                        pf.push_back(itemField);
                    }
                    if (pf.size() >= 6) {
                        PartItem p;
                        p.partId = pf[0];
                        p.name = pf[1];
                        p.quantity = toIntSafe(pf[2]);
                        p.unitPrice = toDoubleSafe(pf[3]);
                        p.price = toDoubleSafe(pf[4]);
                        p.isFromAppointment = (toIntSafe(pf[5]) != 0);
                        v.parts.push_back(p);
                    }
                }
            }
        }
        else {
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
            << a.totalFee << "|"
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
        if (f.size() < 7) continue;

        Appointment a;
        a.appointmentId = f[0];
        a.customerId = f[1];
        a.licensePlate = f[2];
        a.date = f[3];
        a.time = f[4];

        bool hasTotalFee = (f.size() >= 8);
        size_t serviceTypeEnd = hasTotalFee ? f.size() - 2 : f.size() - 1;
        string serviceType = f[5];
        for (size_t i = 6; i < serviceTypeEnd; i++) {
            serviceType += "|" + f[i];
        }
        a.serviceType = serviceType;

        if (hasTotalFee) {
            a.totalFee = toDoubleSafe(f[f.size() - 2]);
        }
        else {
            a.totalFee = 0.0;
        }
        a.isActive = (toIntSafe(f.back()) != 0);
        appointments.push_back(a);
    }
    inFile.close();
}

// ===================================================
// INVOICE FILE I/O
// ===================================================

void saveInvoicesToFile(const vector<Invoice>& invoices) {
    ofstream outFile("invoices.txt");
    if (!outFile.is_open()) {
        cout << "[ERROR] Could not open invoices.txt for writing!\n";
        return;
    }

    for (const auto& inv : invoices) {
        outFile << "INV|" << inv.invoiceId << "|" << inv.customerId << "|"
            << inv.licensePlate << "|" << inv.date << "|"
            << inv.partsCost << "|" << inv.laborCost << "|"
            << inv.discountAmount << "|" << inv.taxAmount << "|"
            << inv.totalAmount << "|" << (inv.isPaid ? 1 : 0) << "|"
            << inv.paymentType << "\n";

        for (const auto& item : inv.items) {
            outFile << "ITEM|" << item.description << "|" << item.itemType << "|"
                << item.quantity << "|" << item.unitPrice << "|"
                << item.totalPrice << "\n";
        }
    }
    outFile.close();
}

void loadInvoicesFromFile(vector<Invoice>& invoices) {
    ifstream inFile("invoices.txt");
    if (!inFile.is_open()) return;

    invoices.clear();
    string line;
    Invoice* currentInvoice = nullptr;

    while (getline(inFile, line)) {
        if (line.empty()) continue;
        vector<string> f = splitFields(line);
        if (f.empty()) continue;

        if (f[0] == "INV" && f.size() >= 12) {
            Invoice inv;
            inv.invoiceId = f[1];
            inv.customerId = f[2];
            inv.licensePlate = f[3];
            inv.date = f[4];
            inv.partsCost = toDoubleSafe(f[5]);
            inv.laborCost = toDoubleSafe(f[6]);
            inv.discountAmount = toDoubleSafe(f[7]);
            inv.taxAmount = toDoubleSafe(f[8]);
            inv.totalAmount = toDoubleSafe(f[9]);
            inv.isPaid = (toIntSafe(f[10]) != 0);
            inv.paymentType = f[11];

            invoices.push_back(inv);
            currentInvoice = &invoices.back();
        }
        else if (f[0] == "ITEM" && f.size() >= 6 && currentInvoice != nullptr) {
            InvoiceItem item;
            item.description = f[1];
            item.itemType = f[2];
            item.quantity = toIntSafe(f[3]);
            item.unitPrice = toDoubleSafe(f[4]);
            item.totalPrice = toDoubleSafe(f[5]);

            currentInvoice->items.push_back(item);
        }
    }
    inFile.close();
}

// ===================================================
// COMBINED HELPERS
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