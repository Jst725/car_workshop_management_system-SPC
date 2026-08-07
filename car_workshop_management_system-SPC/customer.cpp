#include "customer.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
#include <limits>

using namespace std;

bool isValidName(const string& str) {
    if (str.empty()) return false;
    for (size_t i = 0; i < str.length(); i++) {
        if (!isalpha(static_cast<unsigned char>(str[i])) && str[i] != ' ') {
            return false;
        }
    }
    return true;
}

bool isValidPhone(const string& str) {
    if (str.length() < 10 || str.length() > 11) return false;
    for (size_t i = 0; i < str.length(); i++) {
        if (!isdigit(static_cast<unsigned char>(str[i]))) return false;
    }
    return true;
}

bool isValidPlate(const string& str) {
    bool hasLetter = false;
    bool hasDigit = false;
    if (str.empty()) return false;
    for (size_t i = 0; i < str.length(); i++) {
        if (isalpha(static_cast<unsigned char>(str[i]))) hasLetter = true;
        else if (isdigit(static_cast<unsigned char>(str[i]))) hasDigit = true;
        else return false;
    }
    return (hasLetter && hasDigit);
}

bool isValidVehicleText(const string& str) {
    if (str.empty()) return false;
    bool hasLetterOrDigit = false;
    for (size_t i = 0; i < str.length(); i++) {
        unsigned char ch = str[i];
        if (isalpha(ch) || isdigit(ch)) {
            hasLetterOrDigit = true;
        }
        else if (ch == ' ' || ch == '-') {
        }
        else {
            return false;
        }
    }
    return hasLetterOrDigit;
}

static string generateNextCustomerId(const vector<Customer>& customers) {
    int maxNum = 100;
    for (const auto& c : customers) {
        if (c.id.rfind("CUS", 0) == 0) {
            try {
                int num = stoi(c.id.substr(3));
                if (num > maxNum) maxNum = num;
            }
            catch (...) {
            }
        }
    }
    return "CUS" + to_string(maxNum + 1);
}

bool isPlateTaken(const vector<Customer>& customers, const string& plate) {
    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (v.licensePlate == plate) return true;
        }
    }
    return false;
}

void customerMenu(vector<Customer>& customers, vector<Appointment>& appointments, vector<Invoice>& invoices) {
    int choice = -1;
    do {
        cout << "\n========================================\n";
        cout << "         CUSTOMER MANAGEMENT            \n";
        cout << "========================================\n";
        cout << "1. Register New Customer\n";
        cout << "2. Add Vehicle to Customer\n";
        cout << "3. View All Customers\n";
        cout << "4. Search Customer\n";
        cout << "5. Delete Customer\n";
        cout << "0. Return to Main Menu\n";
        cout << "----------------------------------------\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cout << "Invalid input!\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
        case 1: registerCustomer(customers); saveCustomersToFile(customers); break;
        case 2: addVehicleToCustomer(customers); saveCustomersToFile(customers); break;
        case 3: viewCustomers(customers); break;
        case 4: searchCustomer(customers); break;
        case 5: deleteCustomer(customers, appointments, invoices); saveCustomersToFile(customers); break;
        case 0: cout << "Returning to Main Menu...\n"; break;
        default: cout << "Invalid choice!\n";
        }
    } while (choice != 0);
}

void registerCustomer(vector<Customer>& customers) {
    cout << "\n>>> REGISTER NEW CUSTOMER <<<\n";

    Customer c;

    while (true) {
        cout << "Enter Phone Number (or 0 to Return to Main Menu): ";
        cin >> c.phone;
        if (c.phone == "0") return;

        if (!isValidPhone(c.phone)) {
            cout << "[ERROR] Invalid phone number! Must be 10-11 digits, numbers only.\n";
            continue;
        }

        bool duplicate = false;
        for (const auto& existing : customers) {
            if (existing.phone == c.phone) { duplicate = true; break; }
        }
        if (duplicate) {
            cout << "[ERROR] This phone number is already registered!\n";
            continue;
        }
        break;
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    while (true) {
        cout << "Enter Customer Name (Letters Only): ";
        getline(cin, c.name);
        if (isValidName(c.name)) break;
        cout << "[ERROR] Invalid name! Letters and spaces only.\n";
    }

    int tierChoice = 0;
    while (true) {
        cout << "Select Membership Tier:\n";
        cout << "1. Standard\n2. Silver\n3. Gold\n";
        cout << "Choice: ";
        if (!(cin >> tierChoice) || tierChoice < 1 || tierChoice > 3) {
            cout << "[ERROR] Invalid choice!\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }
        break;
    }
    if (tierChoice == 1) c.membershipTier = "Standard";
    else if (tierChoice == 2) c.membershipTier = "Silver";
    else c.membershipTier = "Gold";

    c.id = generateNextCustomerId(customers);
    customers.push_back(c);

    cout << "\n[SUCCESS] Customer registered!\n";
    cout << "Assigned Customer ID: " << c.id << "\n";
    cout << "(Use option 2 - 'Add Vehicle to Customer' to register their vehicle(s).)\n";
}

void addVehicleToCustomer(vector<Customer>& customers) {
    cout << "\n>>> ADD VEHICLE TO CUSTOMER <<<\n";
    if (customers.empty()) {
        cout << "No registered customers in the system yet.\n";
        return;
    }

    string key;
    cout << "Enter Customer ID or Phone Number (or 0 to Return to Main Menu): ";
    cin >> key;
    if (key == "0") return;

    Customer* target = nullptr;
    for (auto& c : customers) {
        if (c.id == key || c.phone == key) { target = &c; break; }
    }

    if (!target) {
        cout << "[ERROR] Customer not found! Please register the customer first.\n";
        return;
    }

    cout << "Customer Found: " << target->name << " (" << target->id << ")\n";
    if (!target->vehicles.empty()) {
        cout << "Existing vehicle(s) on file: ";
        for (size_t i = 0; i < target->vehicles.size(); i++) {
            cout << target->vehicles[i].licensePlate;
            if (i != target->vehicles.size() - 1) cout << ", ";
        }
        cout << "\n";
    }

    Vehicle v;
    while (true) {
        cout << "Enter NEW Vehicle License Plate (Must mix letters & numbers): ";
        cin >> v.licensePlate;
        if (!isValidPlate(v.licensePlate)) {
            cout << "[ERROR] Invalid plate format!\n";
            continue;
        }
        if (isPlateTaken(customers, v.licensePlate)) {
            cout << "[ERROR] This plate is already registered to a customer!\n";
            continue;
        }
        break;
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    while (true) {
        cout << "Enter Vehicle Brand (letters/numbers, spaces and '-' only): ";
        getline(cin, v.brand);
        if (isValidVehicleText(v.brand)) break;
        cout << "[ERROR] Invalid brand! Only letters, numbers, spaces and '-' are allowed.\n";
    }

    while (true) {
        cout << "Enter Vehicle Model (letters/numbers, spaces and '-' only): ";
        getline(cin, v.model);
        if (isValidVehicleText(v.model)) break;
        cout << "[ERROR] Invalid model! Only letters, numbers, spaces and '-' are allowed.\n";
    }

    v.lastServiceDate = "N/A";
    v.status = "Awaiting Parts";
    v.assignedTechnician = "Unassigned";
    v.currentServiceCost = 0.0;

    target->vehicles.push_back(v);
    cout << "\n[SUCCESS] Vehicle " << v.licensePlate << " added to " << target->name
        << ". Total vehicles on file: " << target->vehicles.size() << "\n";
}

void viewCustomers(const vector<Customer>& customers) {
    cout << "\n=================================================================================\n";
    cout << "                              CUSTOMER DIRECTORY                                   \n";
    cout << "=================================================================================\n";

    if (customers.empty()) {
        cout << "No customer records found.\n";
        return;
    }

    int totalVehicles = 0;
    for (size_t i = 0; i < customers.size(); i++) {
        const Customer& c = customers[i];
        cout << "[" << (i + 1) << "] " << left << setw(8) << c.id
            << " | " << setw(20) << c.name
            << " | " << setw(12) << c.phone
            << " | Tier: " << c.membershipTier << "\n";

        if (c.vehicles.empty()) {
            cout << "      Vehicles: (none registered yet)\n";
        }
        else {
            cout << "      Vehicles (" << c.vehicles.size() << "):\n";
            for (const auto& v : c.vehicles) {
                cout << "        - " << left << setw(10) << v.licensePlate
                    << setw(20) << (v.brand + " " + v.model)
                    << "Status: " << v.status << "\n";
                totalVehicles++;
            }
        }
        cout << "---------------------------------------------------------------------------------\n";
    }

    cout << "Total Customers: " << customers.size() << " | Total Vehicles: " << totalVehicles << "\n";
}

void searchCustomer(const vector<Customer>& customers) {
    cout << "\n>>> SEARCH CUSTOMER <<<\n";
    string key;
    cout << "Enter Customer ID or Phone Number (or 0 to Return to Main Menu): ";
    cin >> key;
    if (key == "0") return;

    for (const auto& c : customers) {
        if (c.id == key || c.phone == key) {
            cout << "\n----------------------------------------\n";
            cout << "Customer ID   : " << c.id << "\n";
            cout << "Name          : " << c.name << "\n";
            cout << "Phone         : " << c.phone << "\n";
            cout << "Tier          : " << c.membershipTier << "\n";
            cout << "----------------------------------------\n";
            if (c.vehicles.empty()) {
                cout << "No vehicles registered.\n";
            }
            else {
                cout << "Vehicles:\n";
                for (const auto& v : c.vehicles) {
                    cout << "  - " << v.licensePlate << " (" << v.brand << " " << v.model << ") "
                        << "| Status: " << v.status
                        << " | Accrued Cost: $" << fixed << setprecision(2) << v.currentServiceCost << "\n";
                }
            }
            return;
        }
    }
    cout << "[ERROR] Customer not found.\n";
}

void deleteCustomer(vector<Customer>& customers, const vector<Appointment>& appointments, const vector<Invoice>& invoices) {
    cout << "\n>>> DELETE CUSTOMER <<<\n";
    string custId;
    cout << "Enter Customer ID to delete (or 0 to Return to Main Menu): ";
    cin >> custId;
    if (custId == "0") return;

    for (const auto& a : appointments) {
        if (a.customerId == custId && a.isActive) {
            cout << "[ERROR] This customer has an active appointment (" << a.appointmentId
                << "). Cancel it first before deleting the customer.\n";
            return;
        }
    }
    for (const auto& inv : invoices) {
        if (inv.customerId == custId && !inv.isPaid) {
            cout << "[ERROR] This customer has an unpaid invoice (" << inv.invoiceId
                << "). Settle it first before deleting the customer.\n";
            return;
        }
    }

    for (size_t i = 0; i < customers.size(); i++) {
        if (customers[i].id == custId) {
            cout << "Delete " << customers[i].name << " (" << customers[i].id << ") and "
                << customers[i].vehicles.size() << " linked vehicle(s)? (Y/N): ";
            char confirm;
            cin >> confirm;
            if (confirm == 'Y' || confirm == 'y') {
                customers.erase(customers.begin() + i);
                cout << "[SUCCESS] Customer deleted.\n";
            }
            else {
                cout << "Operation cancelled.\n";
            }
            return;
        }
    }
    cout << "[ERROR] Customer ID not found.\n";
}