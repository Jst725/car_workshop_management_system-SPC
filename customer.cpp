#include "customer.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>

// ===================== CONFIGURATION CONSTANTS =====================
constexpr double TIER_SILVER_THRESHOLD = 1000.0;
constexpr double TIER_GOLD_THRESHOLD = 3000.0;
constexpr double TIER_VIP_THRESHOLD = 5000.0;

constexpr double DISCOUNT_VIP = 0.20;
constexpr double DISCOUNT_GOLD = 0.15;
constexpr double DISCOUNT_SILVER = 0.10;
constexpr double DISCOUNT_NONE = 0.00;

// Static tracker for session maximum ID
static int highestIdEver = 0;

// Helper function to generate CUS0001 formatted ID without reusing deleted IDs
static std::string generateNextCustomerId(const std::vector<Customer>& customers) {
    int maxId = highestIdEver;

    // 1. Check current active customers
    for (const auto& cust : customers) {
        if (cust.id.length() > 3 && cust.id.substr(0, 3) == "CUS") {
            try {
                int num = std::stoi(cust.id.substr(3));
                if (num > maxId) maxId = num;
            }
            catch (...) {}
        }
    }

    // 2. Check invoices.txt history to preserve IDs of deleted customers
    std::ifstream invFile("invoices.txt");
    if (invFile.is_open()) {
        std::string line;
        while (std::getline(invFile, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string invId, custId;
            std::getline(ss, invId, '|');
            std::getline(ss, custId, '|');
            if (custId.length() > 3 && custId.substr(0, 3) == "CUS") {
                try {
                    int num = std::stoi(custId.substr(3));
                    if (num > maxId) maxId = num;
                }
                catch (...) {}
            }
        }
        invFile.close();
    }

    // 3. Check appointments.txt history
    std::ifstream appFile("appointments.txt");
    if (appFile.is_open()) {
        std::string line;
        while (std::getline(appFile, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string appId, custId;
            std::getline(ss, appId, '|');
            std::getline(ss, custId, '|');
            if (custId.length() > 3 && custId.substr(0, 3) == "CUS") {
                try {
                    int num = std::stoi(custId.substr(3));
                    if (num > maxId) maxId = num;
                }
                catch (...) {}
            }
        }
        appFile.close();
    }

    int nextNum = maxId + 1;
    highestIdEver = nextNum;

    // Format into CUS0001 format
    std::stringstream ss;
    ss << "CUS" << std::setw(4) << std::setfill('0') << nextNum;
    return ss.str();
}

// ===================== VERIFICATION HELPER FUNCTIONS =====================

bool isValidName(const std::string& name) {
    if (name.empty()) return false;

    bool hasAlpha = false;
    for (char c : name) {
        if (!std::isalpha(static_cast<unsigned char>(c)) && c != ' ') {
            return false;
        }
        if (std::isalpha(static_cast<unsigned char>(c))) {
            hasAlpha = true;
        }
    }
    return hasAlpha;
}

bool isValidPhone(const std::string& phone) {
    if (phone.length() < 10 || phone.length() > 11) return false;
    for (char c : phone) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

bool isPhoneExists(const std::vector<Customer>& customers, const std::string& phone, const std::string& currentCustId) {
    for (const auto& c : customers) {
        if (c.id != currentCustId && c.phone == phone) {
            return true;
        }
    }
    return false;
}

bool isValidLicensePlate(const std::string& plate) {
    if (plate.empty()) return false;

    bool hasDigit = false;
    bool hasAlpha = false;

    for (char c : plate) {
        if (!std::isalnum(static_cast<unsigned char>(c))) {
            return false;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
        if (std::isalpha(static_cast<unsigned char>(c))) hasAlpha = true;
    }

    return hasDigit && hasAlpha;
}

bool isLicensePlateExists(const std::vector<Customer>& customers, const std::string& plate) {
    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (v.licensePlate == plate) {
                return true;
            }
        }
    }
    return false;
}

// ===================== TIER & DISCOUNT LOGIC =====================

void updateCustomerTier(Customer& customer) {
    std::string previousTier = customer.membershipTier;

    if (customer.totalPurchases >= TIER_VIP_THRESHOLD) {
        customer.membershipTier = "VIP";
    }
    else if (customer.totalPurchases >= TIER_GOLD_THRESHOLD) {
        customer.membershipTier = "Gold";
    }
    else if (customer.totalPurchases >= TIER_SILVER_THRESHOLD) {
        customer.membershipTier = "Silver";
    }
    else {
        customer.membershipTier = "Standard";
    }

    if (previousTier != customer.membershipTier) {
        std::cout << "\n=======================================================\n";
        std::cout << " 🎉 CONGRATULATIONS! TIER UPGRADE ACHIEVED!            \n";
        std::cout << " Customer: " << customer.name << " (" << customer.id << ")\n";
        std::cout << " Upgraded from [" << previousTier << "] to [" << customer.membershipTier << "] Tier!\n";
        std::cout << " Total Lifetime Spending: $" << std::fixed << std::setprecision(2) << customer.totalPurchases << "\n";
        std::cout << "=======================================================\n";
    }
}

double getTierDiscountRate(const std::string& tier) {
    if (tier == "VIP")      return DISCOUNT_VIP;
    if (tier == "Gold")     return DISCOUNT_GOLD;
    if (tier == "Silver")   return DISCOUNT_SILVER;
    return DISCOUNT_NONE;
}

// ===================== CUSTOMER MENU MANAGEMENT =====================

void customerMenu(std::vector<Customer>& customers) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "         CUSTOMER MANAGEMENT            \n";
        std::cout << "========================================\n";
        std::cout << "1. Add New Customer\n";
        std::cout << "2. View All Customers\n";
        std::cout << "3. Search Customer\n";
        std::cout << "4. Update Customer Info\n";
        std::cout << "5. Delete Customer\n";
        std::cout << "0. Return to Main Menu\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Enter choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input! Please enter a number.\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        switch (choice) {
        case 1:
            addCustomer(customers);
            saveCustomersToFile(customers);
            break;
        case 2:
            viewCustomers(customers);
            break;
        case 3:
            searchCustomer(customers);
            break;
        case 4:
            updateCustomer(customers);
            saveCustomersToFile(customers);
            break;
        case 5:
            deleteCustomer(customers);
            saveCustomersToFile(customers);
            break;
        case 0:
            std::cout << "Returning to Main Menu...\n";
            break;
        default:
            std::cout << "Invalid choice!\n";
        }
    } while (choice != 0);
}

void addCustomer(std::vector<Customer>& customers) {
    Customer c;
    c.id = generateNextCustomerId(customers);

    std::cin.ignore(1000, '\n');

    // 1. Name Check
    while (true) {
        std::cout << "\nEnter Customer Name: ";
        std::getline(std::cin, c.name);
        if (!isValidName(c.name)) {
            std::cout << "ERROR: Customer name must contain only alphabetic characters (letters and spaces)!\n";
        }
        else {
            break;
        }
    }

    // 2. Phone Check
    while (true) {
        std::cout << "Enter Phone Number (10-11 digits): ";
        std::getline(std::cin, c.phone);
        if (!isValidPhone(c.phone)) {
            std::cout << "ERROR: Phone number must contain strictly 10 to 11 digits!\n";
        }
        else if (isPhoneExists(customers, c.phone)) {
            std::cout << "ERROR: Phone number is already registered to another customer!\n";
        }
        else {
            break;
        }
    }

    c.membershipTier = "Standard";
    c.totalPurchases = 0.0;

    int vehicleCount = 0;
    while (true) {
        std::cout << "How many vehicles does this customer want to register? ";
        if (std::cin >> vehicleCount && vehicleCount >= 0) break;
        std::cout << "Invalid input! Please enter a non-negative number.\n";
        std::cin.clear();
        std::cin.ignore(1000, '\n');
    }

    for (int i = 0; i < vehicleCount; ++i) {
        Vehicle v;

        // 3. License Plate Check
        while (true) {
            std::cout << "\nVehicle #" << (i + 1) << " License Plate: ";
            std::cin >> v.licensePlate;
            if (!isValidLicensePlate(v.licensePlate)) {
                std::cout << "ERROR: License plate must contain at least 1 letter, at least 1 digit, and no spaces/special characters!\n";
            }
            else if (isLicensePlateExists(customers, v.licensePlate)) {
                std::cout << "ERROR: This license plate is already registered in the system!\n";
            }
            else {
                break;
            }
        }

        std::cin.ignore(1000, '\n');
        std::cout << "Brand (e.g. Toyota / Land Rover): ";
        std::getline(std::cin, v.brand);
        std::cout << "Model (e.g. Camry): ";
        std::getline(std::cin, v.model);

        v.lastServiceDate = "N/A";
        v.status = "Awaiting Parts";
        v.assignedTechnician = "Unassigned";
        v.partsCost = 0.0;
        v.laborCost = 0.0;
        v.currentServiceCost = 0.0;
        c.vehicles.push_back(v);
    }

    customers.push_back(c);
    std::cout << "\nCustomer " << c.name << " registered successfully with ID: " << c.id << "\n";
}

void viewCustomers(const std::vector<Customer>& customers) {
    if (customers.empty()) {
        std::cout << "\nNo customers registered.\n";
        return;
    }

    std::cout << "\n----------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(10) << "ID"
        << std::setw(20) << "Name"
        << std::setw(16) << "Phone"
        << std::setw(12) << "Tier"
        << std::setw(18) << "Lifetime Spent"
        << "Vehicles Registered\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    for (const auto& c : customers) {
        std::cout << std::left << std::setw(10) << c.id
            << std::setw(20) << c.name
            << std::setw(16) << c.phone
            << std::setw(12) << c.membershipTier
            << "$" << std::setw(17) << std::fixed << std::setprecision(2) << c.totalPurchases
            << c.vehicles.size() << "\n";
    }
}

void searchCustomer(const std::vector<Customer>& customers) {
    std::string query;
    std::cout << "\nEnter Customer ID, Name, or License Plate (or 0 to Return to Main Menu): ";
    std::cin.ignore(1000, '\n');
    std::getline(std::cin, query);

    if (query == "0") return;

    std::string upperQuery = query;
    std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);

    bool found = false;
    for (const auto& c : customers) {
        std::string upperCustId = c.id;
        std::string upperCustName = c.name;
        std::transform(upperCustId.begin(), upperCustId.end(), upperCustId.begin(), ::toupper);
        std::transform(upperCustName.begin(), upperCustName.end(), upperCustName.begin(), ::toupper);

        bool plateMatched = false;
        for (const auto& v : c.vehicles) {
            std::string upperPlate = v.licensePlate;
            std::transform(upperPlate.begin(), upperPlate.end(), upperPlate.begin(), ::toupper);
            if (upperPlate == upperQuery) {
                plateMatched = true;
                break;
            }
        }

        if (upperCustId == upperQuery || upperCustName.find(upperQuery) != std::string::npos || plateMatched) {
            found = true;
            std::cout << "\n========================================\n";
            std::cout << "          CUSTOMER DETAILS              \n";
            std::cout << "========================================\n";
            std::cout << "Customer ID    : " << c.id << "\n";
            std::cout << "Name           : " << c.name << "\n";
            std::cout << "Phone          : " << c.phone << "\n";
            std::cout << "VIP Tier       : " << c.membershipTier << "\n";
            std::cout << "Total Spent    : $" << std::fixed << std::setprecision(2) << c.totalPurchases << "\n";
            std::cout << "----------------------------------------\n";
            std::cout << "Registered Vehicles (" << c.vehicles.size() << "):\n";
            if (c.vehicles.empty()) {
                std::cout << "  (No vehicles registered)\n";
            }
            else {
                for (size_t i = 0; i < c.vehicles.size(); ++i) {
                    std::cout << "  " << (i + 1) << ". " << c.vehicles[i].licensePlate
                        << " (" << c.vehicles[i].brand << " " << c.vehicles[i].model << ")\n";
                }
            }
            std::cout << "========================================\n";
        }
    }

    if (!found) {
        std::cout << "No matching customer found.\n";
    }
}

void updateCustomer(std::vector<Customer>& customers) {
    std::string custId;
    std::cout << "\nEnter Customer ID to Update (or 0 to Return to Main Menu): ";
    std::cin >> custId;
    if (custId == "0") return;

    for (auto& c : customers) {
        if (c.id == custId) {
            int choice = -1;
            do {
                std::cout << "\n========================================\n";
                std::cout << "   UPDATE CUSTOMER: " << c.name << " (" << c.id << ")\n";
                std::cout << "========================================\n";
                std::cout << "1. Edit Name\n";
                std::cout << "2. Edit Phone Number\n";
                std::cout << "3. Register New Vehicle\n";
                std::cout << "0. Finish & Save\n";
                std::cout << "----------------------------------------\n";
                std::cout << "Choice: ";

                if (!(std::cin >> choice)) {
                    std::cout << "Invalid choice!\n";
                    std::cin.clear();
                    std::cin.ignore(1000, '\n');
                    continue;
                }

                if (choice == 1) {
                    std::string newName;
                    std::cin.ignore(1000, '\n');
                    while (true) {
                        std::cout << "Enter New Name: ";
                        std::getline(std::cin, newName);
                        if (!isValidName(newName)) {
                            std::cout << "ERROR: Name can only contain alphabetic characters (letters and spaces)!\n";
                        }
                        else {
                            break;
                        }
                    }
                    c.name = newName;
                    std::cout << "Name updated successfully!\n";
                }
                else if (choice == 2) {
                    std::string newPhone;
                    std::cin.ignore(1000, '\n');
                    while (true) {
                        std::cout << "Enter New Phone Number (10-11 digits): ";
                        std::getline(std::cin, newPhone);
                        if (!isValidPhone(newPhone)) {
                            std::cout << "ERROR: Phone number must contain strictly 10 to 11 digits!\n";
                        }
                        else if (isPhoneExists(customers, newPhone, c.id)) {
                            std::cout << "ERROR: Phone number is already registered to another customer!\n";
                        }
                        else {
                            break;
                        }
                    }
                    c.phone = newPhone;
                    std::cout << "Phone number updated successfully!\n";
                }
                else if (choice == 3) {
                    Vehicle v;
                    while (true) {
                        std::cout << "Enter License Plate: ";
                        std::cin >> v.licensePlate;
                        if (!isValidLicensePlate(v.licensePlate)) {
                            std::cout << "ERROR: License plate must contain at least 1 letter, at least 1 digit, and no spaces/special characters!\n";
                        }
                        else if (isLicensePlateExists(customers, v.licensePlate)) {
                            std::cout << "ERROR: This license plate is already registered in the system!\n";
                        }
                        else {
                            break;
                        }
                    }
                    std::cin.ignore(1000, '\n');
                    std::cout << "Brand: ";
                    std::getline(std::cin, v.brand);
                    std::cout << "Model: ";
                    std::getline(std::cin, v.model);

                    v.lastServiceDate = "N/A";
                    v.status = "Awaiting Parts";
                    v.assignedTechnician = "Unassigned";
                    v.partsCost = 0.0;
                    v.laborCost = 0.0;
                    v.currentServiceCost = 0.0;
                    c.vehicles.push_back(v);
                    std::cout << "Vehicle " << v.licensePlate << " added to " << c.name << "!\n";
                }

            } while (choice != 0);

            return;
        }
    }
    std::cout << "Customer ID not found.\n";
}

void deleteCustomer(std::vector<Customer>& customers) {
    std::string custId;
    std::cout << "\nEnter Customer ID to Delete (or 0 to Return to Main Menu): ";
    std::cin >> custId;
    if (custId == "0") return;

    for (auto it = customers.begin(); it != customers.end(); ++it) {
        if (it->id == custId) {
            if (!it->vehicles.empty()) {
                std::cout << "ERROR: Cannot delete customer! They still have " << it->vehicles.size()
                    << " registered vehicle(s) in the system.\n";
                return;
            }

            char confirm = 'N';
            std::cout << "Are you sure you want to delete customer [" << it->name << " - " << it->id << "]? (Y/N): ";
            std::cin >> confirm;

            if (confirm == 'Y' || confirm == 'y') {
                std::cout << "Customer " << it->name << " (" << it->id << ") deleted successfully.\n";
                customers.erase(it);
            }
            else {
                std::cout << "Deletion cancelled.\n";
            }
            return;
        }
    }
    std::cout << "Customer ID not found.\n";
}