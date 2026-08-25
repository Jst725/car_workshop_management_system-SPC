#include "billing.h"
#include "appointment.h"
#include "service.h"
#include "customer.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <ctime>

static std::string toUpper(std::string str) {
    for (char& c : str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return str;
}

// Truncate long strings into concise text for receipt/invoice line-items
static std::string shortenText(const std::string& str, size_t maxLen = 30) {
    if (str.length() <= maxLen) return str;
    return str.substr(0, maxLen - 3) + "...";
}

// Helper to split a string by delimiter (e.g., ',' or '/')
static std::vector<std::string> splitString(const std::string& str, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
        // Trim spaces
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            tokens.push_back(token.substr(start, end - start + 1));
        }
        else if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// Get current system date in YYYY-MM-DD format automatically
static std::string getCurrentDate() {
    std::time_t t = std::time(nullptr);
    std::tm now{};

#ifdef _WIN32
    localtime_s(&now, &t);
#else
    now = *std::localtime(&t);
#endif

    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &now);
    return std::string(buffer);
}

// Get tier discount rate based on system membership tiers
static double getTierDiscountRate(const std::string& tier) {
    if (tier == "Silver") return 0.05;  // 5% discount
    if (tier == "Gold")   return 0.10;  // 10% discount
    if (tier == "VIP")    return 0.20;  // 20% discount
    return 0.0;                       // Regular: 0% discount
}

// Helper: Auto-update customer membership tier based on lifetime total purchases/spending
static void updateCustomerTier(Customer& customer) {
    if (customer.totalPurchases >= 5000.0) customer.membershipTier = "VIP";
    else if (customer.totalPurchases >= 2000.0) customer.membershipTier = "Gold";
    else if (customer.totalPurchases >= 800.0)  customer.membershipTier = "Silver";
    else customer.membershipTier = "Regular";
}

// Generate a collision-safe Bill ID by scanning for the highest numeric suffix
static std::string generateNextInvoiceId(const std::vector<Invoice>& invoices) {
    int maxNum = 0;
    for (const auto& inv : invoices) {
        std::string digits;
        for (char c : inv.invoiceId) {
            if (std::isdigit(static_cast<unsigned char>(c))) digits += c;
        }
        if (!digits.empty()) {
            int n = std::stoi(digits);
            if (n > maxNum) maxNum = n;
        }
    }
    return "B00" + std::to_string(maxNum + 1);
}

// Safely clear a bad std::cin state
static void clearCinError() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Print detailed invoice breakdown (Every Service / Problem, Parts Cost, Labor Cost, Unit Price)
static void printInvoiceDetails(const Invoice& inv, const std::string& customerName) {
    std::cout << "\n========================================================================================\n";
    std::cout << "                              OFFICIAL WORKSHOP INVOICE                                 \n";
    std::cout << "========================================================================================\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Bill ID         : " << inv.invoiceId << "\n";
    std::cout << "Customer        : " << customerName << " (Phone: " << inv.customerId << ")\n";
    std::cout << "License Plate   : " << inv.licensePlate << "\n";
    std::cout << "Invoice Date    : " << inv.date << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(8) << "Type"
        << std::setw(32) << "Service / Problem Name"
        << std::setw(6) << "Qty"
        << std::setw(14) << "Parts Cost(RM)"
        << std::setw(14) << "Labor Cost(RM)"
        << "Total Price(RM)\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    if (inv.items.empty()) {
        std::cout << std::left << std::setw(8) << "Part"
            << std::setw(32) << shortenText("General Service & Repair Cost")
            << std::setw(6) << 1
            << std::setw(14) << inv.partsCost
            << std::setw(14) << inv.laborCost
            << inv.partsCost + inv.laborCost << "\n";
    }
    else {
        for (const auto& item : inv.items) {
            std::cout << std::left << std::setw(8) << item.itemType
                << std::setw(32) << shortenText(item.description)
                << std::setw(6) << item.quantity
                << std::setw(14) << item.unitPrice
                << std::setw(14) << 0.00
                << item.totalPrice << "\n";
        }
    }

    double subtotal = inv.partsCost + inv.laborCost;
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << "Subtotal (Parts: RM" << inv.partsCost << " | Labor: RM" << inv.laborCost << "): RM" << subtotal << "\n";
    std::cout << "Discount Amount : -RM" << inv.discountAmount << "\n";
    std::cout << "Service Tax (6%): RM" << inv.taxAmount << "\n";
    std::cout << "TOTAL AMOUNT    : RM" << inv.totalAmount << "\n";
    std::cout << "Payment Status  : " << (inv.isPaid ? "PAID" : "UNPAID") << "\n";
    std::cout << "========================================================================================\n";
}

void billingMenu(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "        BILL & PAYMENT MANAGEMENT        \n";
        std::cout << "========================================\n";
        std::cout << "1. Generate Invoice\n";
        std::cout << "2. Edit Invoice (Apply/Change Discount)\n";
        std::cout << "3. Process Payment (Print Receipt)\n";
        std::cout << "4. Payment History\n";
        std::cout << "0. Back to Main Menu\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Enter your choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input! Please enter a number.\n";
            clearCinError();
            continue;
        }

        switch (choice) {
        case 1:
            generateInvoice(invoices, customers, appointments);
            break;
        case 2:
            editInvoice(invoices, customers);
            break;
        case 3:
            processPayment(invoices, customers, appointments);
            break;
        case 4:
            paymentHistory(customers, invoices);
            break;
        case 0:
            std::cout << "Returning to Main Menu...\n";
            break;
        default:
            std::cout << "Invalid choice! Please select an option between 0 and 4.\n";
        }
    } while (choice != 0);
}

void editInvoice(std::vector<Invoice>& invoices, std::vector<Customer>& customers) {
    std::string bId;
    std::cout << "\nEnter Bill ID to edit: ";
    std::cin >> bId;
    std::string upId = toUpper(bId);

    auto it = std::find_if(invoices.begin(), invoices.end(), [&](const Invoice& inv) {
        return toUpper(inv.invoiceId) == upId;
        });

    if (it == invoices.end()) {
        std::cout << "Bill not found.\n";
        return;
    }

    Invoice& target = *it;

    if (!target.isPaid) {
        std::cout << "\n1. Update Bill Date\n2. Apply / Change Discount Rate\n3. Delete Bill\n0. Back\nEnter choice: ";
        int uc;
        if (!(std::cin >> uc)) { std::cout << "Invalid input!\n"; clearCinError(); return; }

        if (uc == 1) {
            std::cout << "Enter new date (YYYY-MM-DD): ";
            std::cin >> target.date;
            saveInvoicesToFile(invoices);
            std::cout << "Bill date updated and saved successfully.\n";
        }
        else if (uc == 2) {
            std::cout << "\nChoose Discount Option:\n";
            std::cout << "1. Apply Tier-Based Discount (Auto via Customer Membership)\n";
            std::cout << "2. Apply Custom Percentage Discount\n";
            std::cout << "3. Apply Custom Fixed Amount Discount (RM)\n";
            std::cout << "0. Cancel\nEnter choice: ";

            int dChoice;
            if (!(std::cin >> dChoice)) { std::cout << "Invalid input!\n"; clearCinError(); return; }

            double sub = target.partsCost + target.laborCost;
            double discAmount = 0.0;
            double discPercentageShown = 0.0;

            if (dChoice == 1) {
                double discRate = 0.0;
                auto custIt = std::find_if(customers.begin(), customers.end(), [&](const Customer& c) {
                    return c.phone == target.customerId;
                    });
                if (custIt != customers.end()) {
                    discRate = getTierDiscountRate(custIt->membershipTier);
                }
                discAmount = sub * discRate;
                discPercentageShown = discRate * 100.0;
            }
            else if (dChoice == 2) {
                double pct = 0.0;
                std::cout << "Enter discount percentage (e.g., 15 for 15%): ";
                std::cin >> pct;
                if (pct < 0 || pct > 100) {
                    std::cout << "Invalid percentage range!\n";
                    return;
                }
                discAmount = sub * (pct / 100.0);
                discPercentageShown = pct;
            }
            else if (dChoice == 3) {
                std::cout << "Enter fixed discount amount (RM): ";
                std::cin >> discAmount;
                if (discAmount < 0) discAmount = 0;
                if (discAmount > sub) discAmount = sub;
                discPercentageShown = (sub > 0) ? (discAmount / sub) * 100.0 : 0.0;
            }
            else {
                std::cout << "Discount modification cancelled.\n";
                return;
            }

            target.discountAmount = discAmount;
            target.taxAmount = (sub - target.discountAmount) * 0.06;
            target.totalAmount = (sub - target.discountAmount) + target.taxAmount;

            saveInvoicesToFile(invoices);

            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Discount applied (" << discPercentageShown << "% equivalent / RM" << target.discountAmount << " off).\n";
            std::cout << "New Total (incl. 6% Tax): RM" << target.totalAmount << "\n";
            std::cout << "Changes stored to file successfully.\n";
        }
        else if (uc == 3) {
            std::cout << "Confirm Delete? (Y/N): ";
            char confirm;
            std::cin >> confirm;
            if (std::toupper(confirm) == 'Y') {
                invoices.erase(it);
                saveInvoicesToFile(invoices);
                std::cout << "Bill deleted and changes stored to file successfully.\n";
            }
            else {
                std::cout << "Delete cancelled.\n";
            }
        }
    }
    else {
        std::cout << "\nThis bill is already paid.\n";
        std::cout << "1. Update Payment Method\n2. Revert Payment (undo paid status)\n0. Back\nEnter choice: ";
        int pc;
        if (!(std::cin >> pc)) { std::cout << "Invalid input!\n"; clearCinError(); return; }

        if (pc == 1) {
            std::cout << "Enter new method (Cash/Card/E-Wallet): ";
            std::cin >> target.paymentType;
            saveInvoicesToFile(invoices);
            std::cout << "Payment method updated and saved successfully.\n";
        }
        else if (pc == 2) {
            std::cout << "Confirm Revert Payment? (Y/N): ";
            char confirm;
            std::cin >> confirm;
            if (std::toupper(confirm) == 'Y') {
                target.isPaid = false;
                target.paymentType = "Unpaid";

                auto custIt = std::find_if(customers.begin(), customers.end(), [&](Customer& c) {
                    return c.phone == target.customerId;
                    });
                if (custIt != customers.end()) {
                    custIt->totalPurchases -= target.totalAmount;
                    if (custIt->totalPurchases < 0) custIt->totalPurchases = 0;
                    updateCustomerTier(*custIt);
                }

                saveInvoicesToFile(invoices);
                saveCustomersToFile(customers);

                std::cout << "Payment Reverted -> Recalculated Customer Spending -> Recalculated Membership -> Saved to File.\n";
            }
            else {
                std::cout << "Revert cancelled.\n";
            }
        }
    }
}

void paymentHistory(const std::vector<Customer>& customers, const std::vector<Invoice>& invoices) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "              PAYMENT HISTORY            \n";
        std::cout << "========================================\n";
        std::cout << "1. View All Bills\n";
        std::cout << "2. Search Bills (Phone / Plate / Bill ID)\n";
        std::cout << "3. View Customer Billing History\n";
        std::cout << "0. Back\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Enter choice: ";
        if (!(std::cin >> choice)) {
            std::cout << "Invalid input!\n";
            clearCinError();
            continue;
        }

        if (choice == 1) {
            displayInvoices(invoices);
        }
        else if (choice == 2) {
            std::cout << "\n1. Search by Customer Phone\n2. Search by Plate Number\n3. Search by Bill ID\nEnter choice: ";
            int sChoice;
            if (!(std::cin >> sChoice)) { std::cout << "Invalid input!\n"; clearCinError(); continue; }
            if (sChoice >= 1 && sChoice <= 3) {
                std::string sInput;
                std::cout << "Enter search keyword: ";
                std::cin >> sInput;
                std::string key = toUpper(sInput);
                bool found = false;

                for (const auto& inv : invoices) {
                    if ((sChoice == 1 && toUpper(inv.customerId) == key) ||
                        (sChoice == 2 && toUpper(inv.licensePlate) == key) ||
                        (sChoice == 3 && toUpper(inv.invoiceId) == key)) {
                        std::cout << "Found Bill: " << inv.invoiceId << " | Plate: " << inv.licensePlate
                            << " | Total: RM" << std::fixed << std::setprecision(2) << inv.totalAmount
                            << " | Method: " << inv.paymentType
                            << " | Status: " << (inv.isPaid ? "Paid" : "Unpaid") << "\n";
                        found = true;
                    }
                }
                if (!found) std::cout << "No matching bills found.\n";
            }
        }
        else if (choice == 3) {
            viewCustomerHistory(customers, invoices);
        }
        else if (choice != 0) {
            std::cout << "Invalid choice! Please select an option between 0 and 3.\n";
        }
    } while (choice != 0);
}

void generateInvoice(std::vector<Invoice>& invoices, std::vector<Customer>& customers, const std::vector<Appointment>& /*appointments*/) {
    std::string phoneInput;
    std::cout << "\nEnter Customer Phone Number: ";
    std::cin >> phoneInput;

    auto custIt = std::find_if(customers.begin(), customers.end(), [&](Customer& c) {
        return c.phone == phoneInput;
        });

    if (custIt == customers.end()) {
        std::cout << "ERROR: Customer with phone [" << phoneInput << "] not found in system!\n";
        return;
    }

    Customer& targetCustomer = *custIt;

    std::cout << "\n========================================\n";
    std::cout << "        SELECT COMPLETED SERVICE        \n";
    std::cout << "========================================\n";
    std::cout << "Customer: " << targetCustomer.name << "\n\n";

    bool foundValidService = false;
    int indexCounter = 1;
    for (auto& v : targetCustomer.vehicles) {
        if (v.currentServiceCost > 0.0 && v.status == "Ready for Pickup") {
            foundValidService = true;

            // Resolve description priority
            std::string descDisplay = !v.lastPartsUsed.empty() ? v.lastPartsUsed :
                (!v.lastServiceDescription.empty() ? v.lastServiceDescription : "N/A");

            double partsTotal = v.partsCost > 0.0 ? v.partsCost : v.currentServiceCost;

            std::cout << indexCounter << ". S00" << indexCounter << " | " << v.licensePlate
                << " | Service: " << shortenText(descDisplay, 20)
                << " | Parts Cost: RM" << std::fixed << std::setprecision(2) << partsTotal
                << " | Labor: RM0.00"
                << " | Total: RM" << partsTotal << "\n";
            indexCounter++;
        }
    }

    if (!foundValidService) {
        std::cout << "INFO: No completed services with pending bills found for this customer.\n";
        return;
    }

    int choiceIndex;
    std::cout << "\nSelect Service: ";
    if (!(std::cin >> choiceIndex)) {
        std::cout << "Invalid selection.\n";
        clearCinError();
        return;
    }

    Vehicle* targetVehicle = nullptr;
    int currentIdx = 1;
    for (auto& v : targetCustomer.vehicles) {
        if (v.currentServiceCost > 0.0 && v.status == "Ready for Pickup") {
            if (currentIdx == choiceIndex) {
                targetVehicle = &v;
                break;
            }
            currentIdx++;
        }
    }

    if (!targetVehicle) {
        std::cout << "ERROR: Invalid selection index!\n";
        return;
    }

    // Check for existing active pending bill
    for (const auto& existingInv : invoices) {
        if (existingInv.licensePlate == targetVehicle->licensePlate && !existingInv.isPaid) {
            std::cout << "INFO: An active pending bill [" << existingInv.invoiceId << "] already exists for this vehicle plate!\n";
            return;
        }
    }

    Invoice inv;
    inv.invoiceId = generateNextInvoiceId(invoices);
    inv.customerId = targetCustomer.phone;
    inv.licensePlate = targetVehicle->licensePlate;
    inv.date = targetVehicle->lastServiceDate.empty() ? getCurrentDate() : targetVehicle->lastServiceDate;

    // Total parts cost and set labor cost to 0.00
    inv.partsCost = targetVehicle->partsCost > 0.0 ? targetVehicle->partsCost : targetVehicle->currentServiceCost;
    inv.laborCost = 0.0;

    // Build itemized list for every service/problem
    std::string rawDesc = !targetVehicle->lastPartsUsed.empty() ? targetVehicle->lastPartsUsed :
        (!targetVehicle->lastServiceDescription.empty() ? targetVehicle->lastServiceDescription : "Service & Problem Repair");

    // Split multiple services/problems if separated by commas or slashes
    std::vector<std::string> serviceList = splitString(rawDesc, ',');
    if (serviceList.size() <= 1) {
        serviceList = splitString(rawDesc, '/');
    }

    if (serviceList.empty()) {
        serviceList.push_back(rawDesc);
    }

    double costPerService = inv.partsCost / static_cast<double>(serviceList.size());

    for (const auto& serviceName : serviceList) {
        InvoiceItem item;
        item.description = serviceName;
        item.itemType = "Part";
        item.quantity = 1;
        item.unitPrice = costPerService;
        item.totalPrice = costPerService;
        inv.items.push_back(item);
    }

    double subtotal = inv.partsCost + inv.laborCost;
    double discountRate = getTierDiscountRate(targetCustomer.membershipTier);
    inv.discountAmount = subtotal * discountRate;

    double taxableAmount = subtotal - inv.discountAmount;
    inv.taxAmount = taxableAmount * 0.06; // 6% Service Tax
    inv.totalAmount = taxableAmount + inv.taxAmount;
    inv.isPaid = false;
    inv.paymentType = "Unpaid";

    // Add to memory and permanently write to file storage
    invoices.push_back(inv);
    saveInvoicesToFile(invoices);

    std::cout << "\nSUCCESS: Invoice [" << inv.invoiceId << "] generated and saved to storage!\n";

    // Print Full Invoice Details right after generation
    printInvoiceDetails(inv, targetCustomer.name);
}

void processPayment(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments) {
    std::string phoneInput;
    std::cout << "\nEnter Customer Phone Number: ";
    std::cin >> phoneInput;

    auto custIt = std::find_if(customers.begin(), customers.end(), [&](Customer& c) {
        return c.phone == phoneInput;
        });

    if (custIt == customers.end()) {
        std::cout << "ERROR: Customer with phone [" << phoneInput << "] not found!\n";
        return;
    }

    Customer& targetCustomer = *custIt;

    std::cout << "\n========================================\n";
    std::cout << "               SELECT BILL              \n";
    std::cout << "========================================\n";
    std::cout << "Customer: " << targetCustomer.name << "\n\n";

    bool hasUnpaidBills = false;
    int counter = 1;
    for (const auto& inv : invoices) {
        if (inv.customerId == targetCustomer.phone && !inv.isPaid) {
            hasUnpaidBills = true;
            std::cout << counter << ". " << inv.invoiceId << " | " << inv.licensePlate
                << " | RM" << std::fixed << std::setprecision(2) << inv.totalAmount << " | Unpaid\n";
            counter++;
        }
    }

    if (!hasUnpaidBills) {
        std::cout << "INFO: No unpaid bills found for this customer.\n";
        return;
    }

    int choice;
    std::cout << "\nSelect Bill: ";
    if (!(std::cin >> choice)) {
        std::cout << "Invalid input.\n";
        clearCinError();
        return;
    }

    Invoice* selectedInvoice = nullptr;
    int curr = 1;
    for (auto& inv : invoices) {
        if (inv.customerId == targetCustomer.phone && !inv.isPaid) {
            if (curr == choice) {
                selectedInvoice = &inv;
                break;
            }
            curr++;
        }
    }

    if (!selectedInvoice) {
        std::cout << "ERROR: Invalid selection choice!\n";
        return;
    }

    Vehicle* targetVehicle = nullptr;
    for (auto& v : targetCustomer.vehicles) {
        if (v.licensePlate == selectedInvoice->licensePlate) {
            targetVehicle = &v;
            break;
        }
    }

    // Display Invoice breakdown BEFORE processing payment
    std::cout << "\n--- INVOICE REVIEW PRIOR TO PAYMENT ---";
    printInvoiceDetails(*selectedInvoice, targetCustomer.name);

    std::cout << "\n=======================================================\n";
    std::cout << "                    PAYMENT PROCESSING                  \n";
    std::cout << "=======================================================\n";
    std::cout << "Total Bill      : RM" << std::fixed << std::setprecision(2) << selectedInvoice->totalAmount << "\n";

    int payChoice = 1;
    while (true) {
        std::cout << "Select Payment Method:\n1. Cash\n2. Card\n3. E-Wallet\nChoice: ";
        if (std::cin >> payChoice && payChoice >= 1 && payChoice <= 3) {
            break;
        }
        std::cout << "Invalid choice! Select 1-3.\n";
        clearCinError();
    }

    double amountPaid = 0.0;
    double changeGiven = 0.0;

    if (payChoice == 1) {
        selectedInvoice->paymentType = "Cash";
        while (true) {
            std::cout << "Cash Tendered (Amount Given by Customer) : RM";
            if (std::cin >> amountPaid && amountPaid >= selectedInvoice->totalAmount) {
                changeGiven = amountPaid - selectedInvoice->totalAmount;
                break;
            }
            if (amountPaid >= 0.0 && amountPaid < selectedInvoice->totalAmount) {
                std::cout << "Insufficient cash given! Amount must be at least RM" << selectedInvoice->totalAmount << ".\n";
            }
            else {
                std::cout << "Invalid amount. Please enter a valid number.\n";
                clearCinError();
            }
        }
    }
    else if (payChoice == 2) {
        selectedInvoice->paymentType = "Card";
        amountPaid = selectedInvoice->totalAmount;
    }
    else {
        selectedInvoice->paymentType = "E-Wallet";
        amountPaid = selectedInvoice->totalAmount;
    }

    std::string payDate = getCurrentDate();
    std::cout << "Payment Date    : " << payDate << " (Auto-set to current date)\n";

    double balance = selectedInvoice->totalAmount - amountPaid;
    if (balance < 0) balance = 0.0;

    bool fullySettled = (amountPaid + 1e-9 >= selectedInvoice->totalAmount);

    if (fullySettled) {
        selectedInvoice->isPaid = true;
        targetCustomer.totalPurchases += selectedInvoice->totalAmount;
        updateCustomerTier(targetCustomer);

        if (targetVehicle) {
            resetVehicleServiceData(*targetVehicle);
        }

        for (auto& app : appointments) {
            if (app.licensePlate == selectedInvoice->licensePlate && app.isActive) {
                app.isActive = false;
            }
        }
    }

    // Persist all changes across customers, appointments, and invoices
    saveAllData(customers, appointments, invoices);

    // Official Detailed Payment Receipt showing every item cost
    std::cout << "\n========================================================================================\n";
    std::cout << "                                OFFICIAL PAYMENT RECEIPT                                \n";
    std::cout << "========================================================================================\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << " Bill ID        : " << selectedInvoice->invoiceId << "\n";
    std::cout << " Customer       : " << targetCustomer.name << " (Phone: " << targetCustomer.phone << ")\n";
    std::cout << " Plate          : " << selectedInvoice->licensePlate << "\n";
    std::cout << " Payment Date   : " << payDate << "\n";
    std::cout << " Method         : " << selectedInvoice->paymentType << "\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(8) << " Type"
        << std::setw(32) << " Service / Problem Name"
        << std::setw(6) << " Qty"
        << std::setw(14) << " Parts Cost(RM)"
        << std::setw(14) << " Labor Cost(RM)"
        << " Total Price(RM)\n";
    std::cout << "----------------------------------------------------------------------------------------\n";

    if (selectedInvoice->items.empty()) {
        std::cout << " " << std::left << std::setw(7) << "Part"
            << std::setw(32) << shortenText("General Service & Repair Cost")
            << std::setw(6) << 1
            << std::setw(14) << selectedInvoice->partsCost
            << std::setw(14) << selectedInvoice->laborCost
            << selectedInvoice->partsCost + selectedInvoice->laborCost << "\n";
    }
    else {
        for (const auto& item : selectedInvoice->items) {
            std::cout << " " << std::left << std::setw(7) << item.itemType
                << std::setw(32) << shortenText(item.description)
                << std::setw(6) << item.quantity
                << std::setw(14) << item.unitPrice
                << std::setw(14) << 0.00
                << item.totalPrice << "\n";
        }
    }

    double subtotal = selectedInvoice->partsCost + selectedInvoice->laborCost;
    std::cout << "----------------------------------------------------------------------------------------\n";
    std::cout << " Subtotal (Parts: RM" << selectedInvoice->partsCost << " | Labor: RM" << selectedInvoice->laborCost << "): RM" << subtotal << "\n";
    std::cout << " Discount       : -RM" << selectedInvoice->discountAmount << "\n";
    std::cout << " Service Tax 6% : RM" << selectedInvoice->taxAmount << "\n";
    std::cout << " Total Bill     : RM" << selectedInvoice->totalAmount << "\n";
    if (selectedInvoice->paymentType == "Cash") {
        std::cout << " Cash Given     : RM" << amountPaid << "\n";
        std::cout << " Change Due     : RM" << changeGiven << "\n";
    }
    else {
        std::cout << " Amount Paid    : RM" << amountPaid << "\n";
    }
    std::cout << " Status         : " << (fullySettled ? "PAID IN FULL" : "PARTIAL - Balance remaining") << "\n";
    std::cout << "========================================================================================\n";
}

void displayInvoices(const std::vector<Invoice>& invoices) {
    if (invoices.empty()) {
        std::cout << "\nNo bill records found in storage.\n";
        return;
    }

    std::cout << "\n------------------------------------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(10) << "Bill ID"
        << std::setw(10) << "Cust ID"
        << std::setw(12) << "Plate"
        << std::setw(12) << "Subtotal"
        << std::setw(12) << "Discount"
        << std::setw(12) << "Tax(6%)"
        << std::setw(12) << "Total(RM)"
        << std::setw(12) << "Method"
        << "Status\n";
    std::cout << "------------------------------------------------------------------------------------------------------------------\n";

    for (const auto& inv : invoices) {
        double sub = inv.partsCost + inv.laborCost;
        std::cout << std::left << std::setw(10) << inv.invoiceId
            << std::setw(10) << inv.customerId
            << std::setw(12) << inv.licensePlate
            << std::setw(12) << std::fixed << std::setprecision(2) << sub
            << std::setw(12) << inv.discountAmount
            << std::setw(12) << inv.taxAmount
            << std::setw(12) << inv.totalAmount
            << std::setw(12) << inv.paymentType
            << (inv.isPaid ? "Paid" : "Unpaid") << "\n";
    }
}

void viewCustomerHistory(const std::vector<Customer>& customers, const std::vector<Invoice>& invoices) {
    std::string input;
    std::cout << "\nEnter Customer Name, Phone Number, or License Plate (or 0 to Return): ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, input);
    if (input == "0") return;

    std::string searchKey = toUpper(input);
    const Customer* foundCustomer = nullptr;

    for (const auto& c : customers) {
        if (toUpper(c.name) == searchKey || c.phone == searchKey) {
            foundCustomer = &c;
            break;
        }
    }

    if (!foundCustomer) {
        for (const auto& c : customers) {
            for (const auto& v : c.vehicles) {
                if (toUpper(v.licensePlate) == searchKey) {
                    foundCustomer = &c;
                    break;
                }
            }
            if (foundCustomer) break;
        }
    }

    if (!foundCustomer) {
        std::cout << "ERROR: No customer or vehicle record found for [" << searchKey << "]!\n";
        return;
    }

    std::cout << "\n=======================================================\n";
    std::cout << "                CUSTOMER BILLING HISTORY               \n";
    std::cout << "=======================================================\n";
    std::cout << "Name           : " << foundCustomer->name << "\n";
    std::cout << "Phone          : " << foundCustomer->phone << "\n";
    std::cout << "Membership Tier: " << foundCustomer->membershipTier << "\n";
    std::cout << "Total Spending : RM" << std::fixed << std::setprecision(2) << foundCustomer->totalPurchases << "\n";
    std::cout << "-------------------------------------------------------\n";

    std::cout << "TRANSACTION & BILL HISTORY:\n";
    bool hasInvoices = false;
    std::cout << std::left << std::setw(10) << "Bill ID"
        << std::setw(12) << "Plate"
        << std::setw(12) << "Date"
        << std::setw(12) << "Total(RM)"
        << std::setw(12) << "Method"
        << "Status\n";
    std::cout << "-------------------------------------------------------\n";

    for (const auto& inv : invoices) {
        if (inv.customerId == foundCustomer->phone) {
            hasInvoices = true;
            std::cout << std::left << std::setw(10) << inv.invoiceId
                << std::setw(12) << inv.licensePlate
                << std::setw(12) << inv.date
                << std::setw(12) << std::fixed << std::setprecision(2) << inv.totalAmount
                << std::setw(12) << inv.paymentType
                << (inv.isPaid ? "Paid" : "Unpaid") << "\n";
        }
    }

    if (!hasInvoices) {
        std::cout << "No past bill records found for this customer.\n";
    }
    std::cout << "=======================================================\n";
}