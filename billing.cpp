#include "billing.h"
#include "appointment.h"
#include "vehicle.h"
#include "customer.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>

// Helper: Converts any string to UPPERCASE for uniform lookup
static std::string toUpper(std::string str) {
    for (char& c : str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return str;
}

// Helper: Get tier discount rate based on system membership tiers
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

// Helper: Generate a collision-safe Bill ID by scanning for the highest
// existing numeric suffix instead of relying on vector size (which breaks
// after any deletion, since size() no longer reflects the highest ID issued).
static std::string generateNextInvoiceId(const std::vector<Invoice>& invoices) {
    int maxNum = 0;
    for (const auto& inv : invoices) {
        // Expect format "B00<N>" - pull the trailing digits robustly.
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

// Helper: clear a bad std::cin state safely
static void clearCinError() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void billingMenu(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "      BILL & PAYMENT MANAGEMENT         \n";
        std::cout << "========================================\n";
        std::cout << "1. Generate Invoice\n";
        std::cout << "2. Edit Invoice (apply discount)\n";
        std::cout << "3. Process Payment (print receipt)\n";
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

// "2. Edit Invoice (apply discount)" from the menu diagram. Folds in what
// used to be separate Bill Management (update/delete) and Payment
// Management (update/delete payment) flows, branching on whether the
// selected bill is already paid:
//   - Unpaid bill  -> update date, recalculate/apply discount, or delete.
//   - Paid bill    -> update payment method, or revert payment (undoes the
//                     paid status and reverses the customer's spending and
//                     membership tier - equivalent to the old "Delete
//                     Payment" action).
void editInvoice(std::vector<Invoice>& invoices, std::vector<Customer>& customers) {
    std::string bId;
    std::cout << "\nEnter Bill ID to edit: ";
    std::cin >> bId;
    std::string upId = toUpper(bId);

    Invoice* target = nullptr;
    for (auto& inv : invoices) {
        if (toUpper(inv.invoiceId) == upId) target = &inv;
    }
    if (!target) {
        std::cout << "Bill not found.\n";
        return;
    }

    if (!target->isPaid) {
        std::cout << "\n1. Update Bill Date\n2. Apply/Recalculate Discount\n3. Delete Bill\n0. Back\nEnter choice: ";
        int uc;
        if (!(std::cin >> uc)) { std::cout << "Invalid input!\n"; clearCinError(); return; }

        if (uc == 1) {
            std::cout << "Enter new date: ";
            std::cin >> target->date;
            std::cout << "Bill date updated successfully.\n";
        }
        else if (uc == 2) {
            double sub = target->partsCost + target->laborCost;
            double discRate = 0.0;
            for (const auto& c : customers) {
                if (c.phone == target->customerId) {
                    discRate = getTierDiscountRate(c.membershipTier);
                }
            }
            target->discountAmount = sub * discRate;
            target->taxAmount = (sub - target->discountAmount) * 0.06;
            target->totalAmount = (sub - target->discountAmount) + target->taxAmount;
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Discount applied (" << (discRate * 100.0) << "% - current membership tier).\n";
            std::cout << "New Total: RM" << target->totalAmount << "\n";
        }
        else if (uc == 3) {
            std::cout << "Confirm Delete? (Y/N): ";
            char confirm;
            std::cin >> confirm;
            if (confirm == 'Y' || confirm == 'y') {
                std::string idToRemove = target->invoiceId;
                invoices.erase(std::remove_if(invoices.begin(), invoices.end(),
                    [&](const Invoice& inv) { return inv.invoiceId == idToRemove; }), invoices.end());
                std::cout << "Bill deleted successfully.\n";
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
            std::cin >> target->paymentType;
            std::cout << "Payment method updated successfully.\n";
        }
        else if (pc == 2) {
            std::cout << "Confirm Revert Payment? (Y/N): ";
            char confirm;
            std::cin >> confirm;
            if (confirm == 'Y' || confirm == 'y') {
                target->isPaid = false;
                target->paymentType = "Unpaid";
                for (auto& c : customers) {
                    if (c.phone == target->customerId) {
                        c.totalPurchases -= target->totalAmount;
                        if (c.totalPurchases < 0) c.totalPurchases = 0;
                        updateCustomerTier(c);
                    }
                }
                std::cout << "Payment Reverted -> Recalculated Customer Spending -> Recalculated Membership.\n";
            }
            else {
                std::cout << "Revert cancelled.\n";
            }
        }
    }
}

// "4. Payment History" from the menu diagram. Folds in what used to be
// separate "View All Bills", "Search Bill", and "View Customer Billing
// History" entries.
void paymentHistory(const std::vector<Customer>& customers, const std::vector<Invoice>& invoices) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "             PAYMENT HISTORY             \n";
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
            if (sChoice == 1 || sChoice == 2 || sChoice == 3) {
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

    Customer* targetCustomer = nullptr;
    for (auto& c : customers) {
        if (c.phone == phoneInput) {
            targetCustomer = &c;
            break;
        }
    }

    if (!targetCustomer) {
        std::cout << "ERROR: Customer with phone [" << phoneInput << "] not found in system!\n";
        return;
    }

    std::cout << "\n========================================\n";
    std::cout << "      SELECT COMPLETED SERVICE          \n";
    std::cout << "========================================\n";
    std::cout << "Customer: " << targetCustomer->name << "\n\n";

    bool foundValidService = false;
    int indexCounter = 1;
    // Map temporary reference table for choice selection
    for (auto& v : targetCustomer->vehicles) {
        if (v.currentServiceCost > 0.0 && v.status == "Completed") {
            foundValidService = true;
            std::cout << indexCounter << ". S00" << indexCounter << " | " << v.licensePlate << " | " << (v.lastServiceDescription.empty() ? "General Service" : v.lastServiceDescription) << " | RM" << std::fixed << std::setprecision(2) << v.currentServiceCost << "\n";
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

    // Select matched vehicle service vector slot
    Vehicle* targetVehicle = nullptr;
    int currentIdx = 1;
    for (auto& v : targetCustomer->vehicles) {
        if (v.currentServiceCost > 0.0 && v.status == "Completed") {
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

    std::string plate = targetVehicle->licensePlate;

    // Check for existing active pending bill
    for (const auto& existingInv : invoices) {
        if (existingInv.licensePlate == plate && !existingInv.isPaid) {
            std::cout << "INFO: An active pending bill [" << existingInv.invoiceId << "] already exists for this vehicle plate!\n";
            return;
        }
    }

    Invoice inv;
    inv.invoiceId = generateNextInvoiceId(invoices); // collision-safe, was: "B00" + size()+1
    inv.customerId = targetCustomer->phone;
    inv.licensePlate = targetVehicle->licensePlate;
    inv.date = targetVehicle->lastServiceDate;
    inv.partsCost = targetVehicle->partsCost;
    inv.laborCost = targetVehicle->laborCost;

    if (targetVehicle->partsCost > 0.0) {
        InvoiceItem partItem;
        partItem.description = targetVehicle->lastPartsUsed.empty() ? "Replacement Auto Parts" : targetVehicle->lastPartsUsed;
        partItem.itemType = "Part";
        partItem.quantity = 1;
        partItem.unitPrice = targetVehicle->partsCost;
        partItem.totalPrice = targetVehicle->partsCost;
        inv.items.push_back(partItem);
    }

    if (targetVehicle->laborCost > 0.0) {
        InvoiceItem laborItem;
        laborItem.description = targetVehicle->lastServiceDescription.empty() ? "General Mechanic Labor" : targetVehicle->lastServiceDescription;
        laborItem.itemType = "Labor";
        laborItem.quantity = 1;
        laborItem.unitPrice = targetVehicle->laborCost;
        laborItem.totalPrice = targetVehicle->laborCost;
        inv.items.push_back(laborItem);
    }

    double subtotal = inv.partsCost + inv.laborCost;
    double discountRate = getTierDiscountRate(targetCustomer->membershipTier);
    inv.discountAmount = subtotal * discountRate;

    double taxableAmount = subtotal - inv.discountAmount;
    inv.taxAmount = taxableAmount * 0.06; // 6% Service Tax
    inv.totalAmount = taxableAmount + inv.taxAmount;
    inv.isPaid = false;
    inv.paymentType = "Unpaid";

    invoices.push_back(inv);
    saveInvoicesToFile(invoices);

    std::cout << "\n=========================================================================\n";
    std::cout << "                        OFFICIAL WORKSHOP BILL                           \n";
    std::cout << "=========================================================================\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Bill ID         : " << inv.invoiceId << "\n";
    std::cout << "Customer        : " << targetCustomer->name << " (Phone: " << targetCustomer->phone << ")\n";
    std::cout << "Membership Tier : " << targetCustomer->membershipTier << "\n";
    std::cout << "-------------------------------------------------------------------------\n";
    std::cout << "Subtotal        : RM" << subtotal << "\n";
    std::cout << "Discount Rate   : " << (discountRate * 100.0) << "%\n";
    std::cout << "Discount Amount : RM" << inv.discountAmount << "\n";
    std::cout << "Tax (6%)        : RM" << inv.taxAmount << "\n";
    std::cout << "TOTAL AMOUNT    : RM" << inv.totalAmount << "\n";
    std::cout << "Payment Status  : Unpaid\n";
    std::cout << "=========================================================================\n";
}

void processPayment(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments) {
    std::string phoneInput;
    std::cout << "\nEnter Customer Phone Number: ";
    std::cin >> phoneInput;

    Customer* targetCustomer = nullptr;
    for (auto& c : customers) {
        if (c.phone == phoneInput) {
            targetCustomer = &c;
            break;
        }
    }

    if (!targetCustomer) {
        std::cout << "ERROR: Customer with phone [" << phoneInput << "] not found!\n";
        return;
    }

    std::cout << "\n========================================\n";
    std::cout << "               SELECT BILL              \n";
    std::cout << "========================================\n";
    std::cout << "Customer: " << targetCustomer->name << "\n\n";

    bool hasUnpaidBills = false;
    int counter = 1;
    for (const auto& inv : invoices) {
        if (inv.customerId == targetCustomer->phone && !inv.isPaid) {
            hasUnpaidBills = true;
            std::cout << counter << ". " << inv.invoiceId << " | " << inv.licensePlate << " | RM" << std::fixed << std::setprecision(2) << inv.totalAmount << " | Unpaid\n";
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
        if (inv.customerId == targetCustomer->phone && !inv.isPaid) {
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
    for (auto& v : targetCustomer->vehicles) {
        if (v.licensePlate == selectedInvoice->licensePlate) {
            targetVehicle = &v;
            break;
        }
    }

    std::cout << "\n=======================================================\n";
    std::cout << "                 PAYMENT PROCESSING                    \n";
    std::cout << "=======================================================\n";
    std::cout << "Total Bill      : RM" << std::fixed << std::setprecision(2) << selectedInvoice->totalAmount << "\n";

    double amountPaid = 0.0;
    while (true) {
        std::cout << "Amount Paid     : RM";
        if (std::cin >> amountPaid && amountPaid >= 0.0) break;
        std::cout << "Invalid amount. Please enter a non-negative number.\n";
        clearCinError();
    }

    std::string payDate;
    std::cout << "Payment Date    : ";
    std::cin >> payDate;

    int payChoice = 1;
    while (true) {
        std::cout << "Select Payment Method:\n1. Cash\n2. Card\n3. E-Wallet\nChoice: ";
        if (std::cin >> payChoice && payChoice >= 1 && payChoice <= 3) {
            break;
        }
        std::cout << "Invalid choice! Select 1-3.\n";
        clearCinError();
    }

    switch (payChoice) {
    case 1: selectedInvoice->paymentType = "Cash"; break;
    case 2: selectedInvoice->paymentType = "Card"; break;
    case 3: selectedInvoice->paymentType = "E-Wallet"; break;
    }

    // Only fully settle the bill (mark paid, update tier, reset vehicle,
    // close appointment) once the amount actually covers the total.
    // Previously this happened unconditionally regardless of amountPaid,
    // so a partial payment silently closed the bill in full.
    double balance = selectedInvoice->totalAmount - amountPaid;
    if (balance < 0) balance = 0.0;

    bool fullySettled = (amountPaid + 1e-9 >= selectedInvoice->totalAmount);

    if (fullySettled) {
        selectedInvoice->isPaid = true;
        targetCustomer->totalPurchases += selectedInvoice->totalAmount;
        updateCustomerTier(*targetCustomer);

        if (targetVehicle) {
            resetVehicleServiceData(*targetVehicle);
        }

        for (auto& app : appointments) {
            if (app.licensePlate == selectedInvoice->licensePlate && app.isActive) {
                app.isActive = false;
            }
        }
    }

    saveAllData(customers, appointments, invoices);

    std::cout << "\n=======================================================\n";
    std::cout << "                 OFFICIAL PAYMENT RECEIPT              \n";
    std::cout << "=======================================================\n";
    std::cout << " Bill ID      : " << selectedInvoice->invoiceId << "\n";
    std::cout << " Customer     : " << targetCustomer->name << " (Phone: " << targetCustomer->phone << ")\n";
    std::cout << " Plate        : " << selectedInvoice->licensePlate << "\n";
    std::cout << " Payment Date : " << payDate << "\n";
    std::cout << " Method       : " << selectedInvoice->paymentType << "\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << " Total Bill   : RM" << std::fixed << std::setprecision(2) << selectedInvoice->totalAmount << "\n";
    std::cout << " Amount Paid  : RM" << amountPaid << "\n";
    std::cout << " Balance      : RM" << balance << "\n";
    std::cout << " Status       : " << (fullySettled ? "PAID IN FULL" : "PARTIAL - Balance remaining") << "\n";
    std::cout << "=======================================================\n";
}

void displayInvoices(const std::vector<Invoice>& invoices) {
    if (invoices.empty()) {
        std::cout << "\nNo bill records found in storage.\n";
        return;
    }

    std::cout << "\n---------------------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(10) << "Bill ID"
        << std::setw(10) << "Cust ID"
        << std::setw(12) << "Plate"
        << std::setw(12) << "Subtotal"
        << std::setw(12) << "Discount"
        << std::setw(12) << "Total(RM)"
        << std::setw(12) << "Method"
        << "Status\n";
    std::cout << "---------------------------------------------------------------------------------------------------\n";

    for (const auto& inv : invoices) {
        double sub = inv.partsCost + inv.laborCost;
        std::cout << std::left << std::setw(10) << inv.invoiceId
            << std::setw(10) << inv.customerId
            << std::setw(12) << inv.licensePlate
            << std::setw(12) << std::fixed << std::setprecision(2) << sub
            << std::setw(12) << inv.discountAmount
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
    std::cout << "               CUSTOMER BILLING HISTORY                \n";
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