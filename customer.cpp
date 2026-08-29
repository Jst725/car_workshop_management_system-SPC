#include "customer.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cctype>
#include <limits>
#include <cstdlib>

using namespace std;

// ANSI Light Color Definitions (No Blue, Green, or Purple)
namespace Color {
    const string RESET = "\033[0m";
    const string BOLD = "\033[1m";
    const string CYAN = "\033[36m";
    const string BRIGHT_CYAN = "\033[96m";
    const string BRIGHT_YELLOW = "\033[93m";
    const string BRIGHT_WHITE = "\033[97m";
    const string BRIGHT_RED = "\033[91m";
    const string LIGHT_GRAY = "\033[37m";
}

// Helper: Cross-platform screen clear function
static void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Helper: Convert string to uppercase
static string toUpper(string str) {
    for (char& c : str) {
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }
    return str;
}

// ===================================================
// VALIDATION HELPERS
// ===================================================

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

// Alias so appointment.cpp's call to isValidLicensePlate(...) resolves to the
// exact same rule as isValidPlate (must mix letters & digits, no symbols).
bool isValidLicensePlate(const string& str) {
    return isValidPlate(str);
}

// Brand/model names only ever need letters, digits, spaces and a hyphen
// (e.g. "CX-5", "e-tron", "Model 3", "C-Class"). Anything else (symbols like
// % $ # @ etc.) is rejected so junk can't get saved into the record.
bool isValidVehicleText(const string& str) {
    if (str.empty()) return false;
    bool hasLetterOrDigit = false;
    for (size_t i = 0; i < str.length(); i++) {
        unsigned char ch = str[i];
        if (isalpha(ch) || isdigit(ch)) {
            hasLetterOrDigit = true;
        }
        else if (ch == ' ' || ch == '-') {
            // allowed separator characters
        }
        else {
            return false;
        }
    }
    return hasLetterOrDigit; // reject strings that are only spaces/hyphens
}

// Checks the plate across EVERY customer's vehicle list (not just one customer),
// since license plates must be unique system-wide.
bool isPlateTaken(const vector<Customer>& customers, const string& plate) {
    string upperPlate = toUpper(plate);
    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (v.licensePlate == upperPlate) return true;
        }
    }
    return false;
}

// ===================================================
// MENU
// ===================================================

void customerMenu(vector<Customer>& customers) {
    int choice = -1;
    do {
        clearScreen();
        cout << Color::CYAN << "\n========================================\n" << Color::RESET;
        cout << Color::BRIGHT_YELLOW << Color::BOLD << "         CUSTOMER MANAGEMENT            \n" << Color::RESET;
        cout << Color::CYAN << "========================================\n" << Color::RESET;
        cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Register New Customer\n";
        cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Add Vehicle to Customer\n";
        cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "View All Customers\n";
        cout << Color::BRIGHT_CYAN << "4. " << Color::BRIGHT_WHITE << "Search Customer\n";
        cout << Color::BRIGHT_CYAN << "5. " << Color::BRIGHT_WHITE << "Edit Customer\n";
        cout << Color::BRIGHT_CYAN << "6. " << Color::BRIGHT_WHITE << "Delete Customer\n";
        cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Return to Main Menu\n";
        cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
        cout << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;

        if (!(cin >> choice)) {
            cout << Color::BRIGHT_RED << "Invalid input!\n" << Color::RESET;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
        case 1:
            clearScreen();
            registerCustomer(customers);
            saveCustomersToFile(customers);
            break;
        case 2:
            clearScreen();
            addVehicleToCustomer(customers);
            saveCustomersToFile(customers);
            break;
        case 3:
            clearScreen();
            viewCustomers(customers);
            break;
        case 4:
            clearScreen();
            searchCustomer(customers);
            break;
        case 5:
            clearScreen();
            editCustomer(customers);
            saveCustomersToFile(customers);
            break;
        case 6:
            clearScreen();
            deleteCustomer(customers);
            saveCustomersToFile(customers);
            break;
        case 0:
            cout << Color::BRIGHT_CYAN << "Returning to Main Menu...\n" << Color::RESET;
            break;
        default:
            cout << Color::BRIGHT_RED << "Invalid choice!\n" << Color::RESET;
        }
    } while (choice != 0);
}

// ===================================================
// REGISTER CUSTOMER
// (Phone number is the unique identifier - no more Customer ID)
// ===================================================

void registerCustomer(vector<Customer>& customers) {
    cout << Color::BRIGHT_YELLOW << Color::BOLD << "\n>>> REGISTER NEW CUSTOMER <<<\n" << Color::RESET;

    Customer c;

    // ---- Phone (unique identifier) ----
    while (true) {
        cout << Color::BRIGHT_YELLOW << "Enter Phone Number (or 0 to Return to Main Menu): " << Color::RESET;
        cin >> c.phone;
        if (c.phone == "0") return;

        if (!isValidPhone(c.phone)) {
            cout << Color::BRIGHT_RED << "[ERROR] Invalid phone number! Must be 10-11 digits, numbers only.\n" << Color::RESET;
            continue;
        }

        bool duplicate = false;
        for (const auto& existing : customers) {
            if (existing.phone == c.phone) { duplicate = true; break; }
        }
        if (duplicate) {
            cout << Color::BRIGHT_RED << "[ERROR] This phone number is already registered!\n" << Color::RESET;
            continue;
        }
        break;
    }

    // ---- Name ----
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    while (true) {
        cout << Color::BRIGHT_YELLOW << "Enter Customer Name (Letters Only): " << Color::RESET;
        getline(cin, c.name);
        if (isValidName(c.name)) break;
        cout << Color::BRIGHT_RED << "[ERROR] Invalid name! Letters and spaces only.\n" << Color::RESET;
    }

    // ---- Membership Tier ----
    // NOTE: Tier is normally auto-upgraded by billing.cpp based on totalPurchases
    // (Regular -> Silver -> Gold -> VIP). New customers start as Regular.
    c.membershipTier = "Regular";
    c.totalPurchases = 0.0;

    customers.push_back(c);

    cout << Color::BRIGHT_CYAN << "\n[SUCCESS] Customer registered!\n" << Color::RESET;
    cout << Color::BRIGHT_WHITE << "Phone Number (use this to identify the customer): " << c.phone << "\n";
    cout << "Membership Tier: Regular (upgrades automatically as they spend more)\n";
    cout << "(Use option 2 - 'Add Vehicle to Customer' to register their vehicle(s).)\n" << Color::RESET;
}

// ===================================================
// ADD VEHICLE (supports MULTIPLE vehicles per customer)
// ===================================================

void addVehicleToCustomer(vector<Customer>& customers) {
    cout << Color::BRIGHT_YELLOW << Color::BOLD << "\n>>> ADD VEHICLE TO CUSTOMER <<<\n" << Color::RESET;
    if (customers.empty()) {
        cout << Color::BRIGHT_YELLOW << "No registered customers in the system yet.\n" << Color::RESET;
        return;
    }

    string phone;
    cout << Color::BRIGHT_YELLOW << "Enter Customer Phone Number (or 0 to Return to Main Menu): " << Color::RESET;
    cin >> phone;
    if (phone == "0") return;

    Customer* target = nullptr;
    for (auto& c : customers) {
        if (c.phone == phone) { target = &c; break; }
    }

    if (!target) {
        cout << Color::BRIGHT_RED << "[ERROR] Customer not found! Please register the customer first.\n" << Color::RESET;
        return;
    }

    cout << Color::BRIGHT_CYAN << "Customer Found: " << Color::BRIGHT_WHITE << target->name << " (" << target->phone << ")\n" << Color::RESET;
    if (!target->vehicles.empty()) {
        cout << Color::BRIGHT_CYAN << "Existing vehicle(s) on file: " << Color::BRIGHT_WHITE;
        for (size_t i = 0; i < target->vehicles.size(); i++) {
            cout << target->vehicles[i].licensePlate;
            if (i != target->vehicles.size() - 1) cout << ", ";
        }
        cout << "\n" << Color::RESET;
    }

    Vehicle v;
    while (true) {
        cout << Color::BRIGHT_YELLOW << "\nEnter New Vehicle License Plate: " << Color::RESET;
        cin >> v.licensePlate;
        v.licensePlate = toUpper(v.licensePlate);

        if (!isValidPlate(v.licensePlate)) {
            cout << Color::BRIGHT_RED << "ERROR: License plate must contain at least 1 letter, at least 1 digit, and no spaces/special characters!\n" << Color::RESET;
        }
        else if (isPlateTaken(customers, v.licensePlate)) {
            cout << Color::BRIGHT_RED << "ERROR: This license plate is already registered to another customer!\n" << Color::RESET;
        }
        else {
            break;
        }
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    cout << Color::BRIGHT_YELLOW << "Enter Vehicle Brand (e.g. Honda): " << Color::RESET;
    getline(cin, v.brand);
    v.brand = toUpper(v.brand);

    cout << Color::BRIGHT_YELLOW << "Enter Vehicle Model (e.g. Civic): " << Color::RESET;
    getline(cin, v.model);
    v.model = toUpper(v.model);

    v.lastServiceDate = "N/A";
    v.status = "Awaiting Parts";
    v.assignedTechnician = "Unassigned";
    v.lastPartsUsed = "";
    v.lastServiceDescription = "";
    v.partsCost = 0.0;
    v.laborCost = 0.0;
    v.currentServiceCost = 0.0;

    target->vehicles.push_back(v);
    cout << Color::BRIGHT_CYAN << "\n[SUCCESS] Vehicle " << Color::BRIGHT_WHITE << v.licensePlate << Color::BRIGHT_CYAN << " added to " << Color::BRIGHT_WHITE << target->name
        << Color::BRIGHT_CYAN << ". Total vehicles on file: " << target->vehicles.size() << "\n" << Color::RESET;
}

// ===================================================
// VIEW ALL CUSTOMERS (shows EVERY vehicle per customer)
// ===================================================

void viewCustomers(const vector<Customer>& customers) {
    cout << Color::CYAN << "\n============================================================================================\n" << Color::RESET;
    cout << Color::BRIGHT_YELLOW << Color::BOLD << "                                    CUSTOMER DIRECTORY                                   \n" << Color::RESET;
    cout << Color::CYAN << "============================================================================================\n" << Color::RESET;

    if (customers.empty()) {
        cout << Color::BRIGHT_YELLOW << "No customer records found.\n" << Color::RESET;
    }
    else {
        int totalVehicles = 0;
        for (size_t i = 0; i < customers.size(); i++) {
            const Customer& c = customers[i];
            cout << Color::BRIGHT_CYAN << "[" << (i + 1) << "] " << Color::BRIGHT_WHITE << left << setw(20) << c.name
                << Color::BRIGHT_CYAN << " | Phone: " << Color::BRIGHT_WHITE << setw(10) << c.phone
                << Color::BRIGHT_CYAN << " | Tier: " << Color::BRIGHT_WHITE << setw(8) << c.membershipTier
                << Color::BRIGHT_CYAN << " | Total Spent: RM" << Color::BRIGHT_WHITE << fixed << setprecision(2) << c.totalPurchases << "\n";

            if (c.vehicles.empty()) {
                cout << Color::LIGHT_GRAY << "      Vehicles: (none registered yet)\n" << Color::RESET;
            }
            else {
                cout << Color::BRIGHT_CYAN << "      Vehicles (" << c.vehicles.size() << "):\n" << Color::RESET;
                for (const auto& v : c.vehicles) {
                    cout << Color::BRIGHT_WHITE << "        - " << left << setw(10) << v.licensePlate
                        << setw(20) << (v.brand + " " + v.model)
                        << Color::BRIGHT_CYAN << "Status: " << Color::BRIGHT_WHITE << v.status << "\n";
                    totalVehicles++;
                }
            }
            cout << Color::CYAN << "--------------------------------------------------------------------------------------------\n" << Color::RESET;
        }

        cout << Color::BRIGHT_YELLOW << "Total Customers: " << customers.size() << " | Total Vehicles: " << totalVehicles << "\n" << Color::RESET;
    }

    cout << Color::BRIGHT_YELLOW << "\nPress Enter to return to the menu..." << Color::RESET;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

// ===================================================
// SEARCH CUSTOMER (by phone number only)
// ===================================================

void searchCustomer(const vector<Customer>& customers) {
    cout << Color::BRIGHT_YELLOW << Color::BOLD << "\n>>> SEARCH CUSTOMER <<<\n" << Color::RESET;
    string phone;
    cout << Color::BRIGHT_YELLOW << "Enter Phone Number (or 0 to Return to Main Menu): " << Color::RESET;
    cin >> phone;
    if (phone == "0") return;

    bool found = false;
    for (const auto& c : customers) {
        if (c.phone == phone) {
            found = true;
            cout << Color::CYAN << "\n----------------------------------------\n" << Color::RESET;
            cout << Color::BRIGHT_CYAN << "Name          : " << Color::BRIGHT_WHITE << c.name << "\n";
            cout << Color::BRIGHT_CYAN << "Phone         : " << Color::BRIGHT_WHITE << c.phone << "\n";
            cout << Color::BRIGHT_CYAN << "Tier          : " << Color::BRIGHT_WHITE << c.membershipTier << "\n";
            cout << Color::BRIGHT_CYAN << "Total Spent   : RM" << Color::BRIGHT_WHITE << fixed << setprecision(2) << c.totalPurchases << "\n";
            cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
            if (c.vehicles.empty()) {
                cout << Color::BRIGHT_YELLOW << "No vehicles registered.\n" << Color::RESET;
            }
            else {
                cout << Color::BRIGHT_CYAN << "Vehicles:\n" << Color::RESET;
                for (const auto& v : c.vehicles) {
                    cout << Color::BRIGHT_WHITE << "  - " << v.licensePlate << " (" << v.brand << " " << v.model << ") "
                        << Color::BRIGHT_CYAN << "| Status: " << Color::BRIGHT_WHITE << v.status
                        << Color::BRIGHT_CYAN << " | Accrued Cost: RM" << Color::BRIGHT_WHITE << fixed << setprecision(2) << v.currentServiceCost << "\n";
                }
            }
            break;
        }
    }

    if (!found) {
        cout << Color::BRIGHT_RED << "[ERROR] Customer not found.\n" << Color::RESET;
    }

    // Pause so the search results don't clear instantly
    cout << Color::BRIGHT_YELLOW << "\nPress Enter to return to the menu..." << Color::RESET;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

// ===================================================
// EDIT CUSTOMER (by phone number)
// Only non-vehicle fields can be changed here (Name, Membership Tier).
// Phone number is NOT editable - it's the key used to look up this customer
// everywhere else in the system (appointments, invoices), so changing it here
// would silently break those links elsewhere.
// ===================================================

void editCustomer(vector<Customer>& customers) {
    cout << Color::BRIGHT_YELLOW << Color::BOLD << "\n>>> EDIT CUSTOMER <<<\n" << Color::RESET;
    string phone;
    cout << Color::BRIGHT_YELLOW << "Enter Phone Number of the customer to edit (or 0 to Return to Main Menu): " << Color::RESET;
    cin >> phone;
    if (phone == "0") return;

    Customer* target = nullptr;
    for (auto& c : customers) {
        if (c.phone == phone) { target = &c; break; }
    }

    if (!target) {
        cout << Color::BRIGHT_RED << "[ERROR] Customer not found.\n" << Color::RESET;
        return;
    }

    int choice = -1;
    do {
        cout << Color::CYAN << "\n----------------------------------------\n" << Color::RESET;
        cout << Color::BRIGHT_YELLOW << Color::BOLD << "Editing: " << target->name << " (" << target->phone << ")\n" << Color::RESET;
        cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
        cout << Color::BRIGHT_CYAN << "Name          : " << Color::BRIGHT_WHITE << target->name << "\n";
        cout << Color::BRIGHT_CYAN << "Phone         : " << Color::BRIGHT_WHITE << target->phone << "\n";
        cout << Color::BRIGHT_CYAN << "Tier          : " << Color::BRIGHT_WHITE << target->membershipTier << "\n";
        cout << Color::BRIGHT_CYAN << "Total Spent   : RM" << Color::BRIGHT_WHITE << fixed << setprecision(2) << target->totalPurchases << "\n";
        cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
        cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Edit Name\n";
        cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Edit Membership Tier\n";
        cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Done\n";
        cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
        cout << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;

        if (!(cin >> choice)) {
            cout << Color::BRIGHT_RED << "Invalid input!\n" << Color::RESET;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
        case 1: {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            string newName;
            while (true) {
                cout << Color::BRIGHT_YELLOW << "Enter New Name (Letters Only): " << Color::RESET;
                getline(cin, newName);
                if (isValidName(newName)) break;
                cout << Color::BRIGHT_RED << "[ERROR] Invalid name! Letters and spaces only.\n" << Color::RESET;
            }
            target->name = newName;
            cout << Color::BRIGHT_CYAN << "[SUCCESS] Name updated.\n" << Color::RESET;
            break;
        }
        case 2: {
            int tierChoice = 0;
            while (true) {
                cout << Color::BRIGHT_YELLOW << "Select New Membership Tier:\n" << Color::RESET;
                cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Regular\n";
                cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Silver\n";
                cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Gold\n";
                cout << Color::BRIGHT_CYAN << "4. " << Color::BRIGHT_WHITE << "VIP\n";
                cout << Color::BRIGHT_YELLOW << "Choice: " << Color::RESET;
                if (!(cin >> tierChoice) || tierChoice < 1 || tierChoice > 4) {
                    cout << Color::BRIGHT_RED << "[ERROR] Invalid choice!\n" << Color::RESET;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    continue;
                }
                break;
            }
            if (tierChoice == 1) target->membershipTier = "Regular";
            else if (tierChoice == 2) target->membershipTier = "Silver";
            else if (tierChoice == 3) target->membershipTier = "Gold";
            else target->membershipTier = "VIP";
            cout << Color::BRIGHT_CYAN << "[SUCCESS] Membership tier updated.\n" << Color::RESET;
            break;
        }
        case 0:
            cout << Color::BRIGHT_CYAN << "Finished editing.\n" << Color::RESET;
            break;
        default:
            cout << Color::BRIGHT_RED << "Invalid choice!\n" << Color::RESET;
        }
    } while (choice != 0);
}

// ===================================================
// DELETE CUSTOMER (by phone number)
// ===================================================

void deleteCustomer(vector<Customer>& customers) {
    cout << Color::BRIGHT_YELLOW << Color::BOLD << "\n>>> DELETE CUSTOMER <<<\n" << Color::RESET;
    string phone;
    cout << Color::BRIGHT_YELLOW << "Enter Phone Number to delete (or 0 to Return to Main Menu): " << Color::RESET;
    cin >> phone;
    if (phone == "0") return;

    for (size_t i = 0; i < customers.size(); i++) {
        if (customers[i].phone == phone) {
            cout << Color::BRIGHT_YELLOW << "Delete " << Color::BRIGHT_WHITE << customers[i].name << " (" << customers[i].phone << ") and "
                << customers[i].vehicles.size() << " linked vehicle(s)? (Y/N): " << Color::RESET;
            char confirm;
            cin >> confirm;
            if (confirm == 'Y' || confirm == 'y') {
                customers.erase(customers.begin() + i);
                cout << Color::BRIGHT_CYAN << "[SUCCESS] Customer deleted.\n" << Color::RESET;
            }
            else {
                cout << Color::BRIGHT_YELLOW << "Operation cancelled.\n" << Color::RESET;
            }
            return;
        }
    }
    cout << Color::BRIGHT_RED << "[ERROR] Phone number not found.\n" << Color::RESET;
}