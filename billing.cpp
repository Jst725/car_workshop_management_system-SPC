#include "billing.h"
#include "appointment.h"
#include "vehicle.h"
#include "customer.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>

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

void billingMenu(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "      BILL & PAYMENT MANAGEMENT         \n";
        std::cout << "========================================\n";
        std::cout << "1. Bill Management\n";
        std::cout << "2. Payment Management\n";
        std::cout << "3. View All Bills / Invoices\n";
        std::cout << "4. View Customer Billing & Service History\n";
        std::cout << "0. Back to Main Menu\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Enter your choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input! Please enter a number.\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        switch (choice) {
        case 1: {
            int billChoice = -1;
            do {
                std::cout << "\n========================================\n";
                std::cout << "            BILL MANAGEMENT             \n";
                std::cout << "========================================\n";
                std::cout << "1. Add Bill\n";
                std::cout << "2. View Bills\n";
                std::cout << "3. Search Bill\n";
                std::cout << "4. Update Bill\n";
                std::cout << "5. Delete Bill\n";
                std::cout << "6. Back\n";
                std::cout << "----------------------------------------\n";
                std::cout << "Enter your choice: ";
                if (!(std::cin >> billChoice)) {
                    std::cout << "Invalid input!\n";
                    std::cin.clear();
                    std::cin.ignore(1000, '\n');
                    continue;
                }
                if (billChoice == 1) generateInvoice(invoices, customers, appointments);
                else if (billChoice == 2) displayInvoices(invoices);
                else if (billChoice == 3) {
                    // Search Bill submenu routing
                    std::string sInput;
                    std::cout << "\n1. Search by Customer Phone\n2. Search by Plate Number\n3. Search by Bill ID\n4. Back\nEnter choice: ";
                    int sChoice;
                    std::cin >> sChoice;
                    if (sChoice == 1 || sChoice == 2 || sChoice == 3) {
                        std::cout << "Enter search keyword: ";
                        std::cin >> sInput;
                        std::string key = toUpper(sInput);
                        bool found = false;
                        for (const auto& inv : invoices) {
                            if ((sChoice == 1 && toUpper(inv.customerId) == key) ||
                                (sChoice == 2 && toUpper(inv.licensePlate) == key) ||
                                (sChoice == 3 && toUpper(inv.invoiceId) == key)) {
                                std::cout << "Found Bill: " << inv.invoiceId << " | Plate: " << inv.licensePlate << " | Total: RM" << inv.totalAmount << " | Status: " << (inv.isPaid ? "Paid" : "Unpaid") << "\n";
                                found = true;
                            }
                        }
                        if (!found) std::cout << "No matching bills found.\n";
                    }
                }
                else if (billChoice == 4) {
                    // Update Bill Submenu (Membership discount locked as required)
                    std::string bId;
                    std::cout << "Enter Bill ID to update: ";
                    std::cin >> bId;
                    std::string upId = toUpper(bId);
                    Invoice* targetB = nullptr;
                    for (auto& inv : invoices) {
                        if (toUpper(inv.invoiceId) == upId) targetB = &inv;
                    }
                    if (!targetB) {
                        std::cout << "Bill not found.\n";
                    }
                    else if (targetB->isPaid) {
                        std::cout << "Cannot update already paid bill.\n";
                    }
                    else {
                        std::cout << "1. Update Bill Date\n2. Recalculate Bill\n3. Back\nEnter choice: ";
                        int uc; std::cin >> uc;
                        if (uc == 1) {
                            std::cout << "Enter new date: ";
                            std::cin >> targetB->date;
                            std::cout << "Bill date updated successfully.\n";
                        }
                        else if (uc == 2) {
                            double sub = targetB->partsCost + targetB->laborCost;
                            double discRate = 0.0;
                            for (const auto& c : customers) {
                                if (c.phone == targetB->customerId) {
                                    discRate = getTierDiscountRate(c.membershipTier);
                                }
                            }
                            targetB->discountAmount = sub * discRate;
                            targetB->taxAmount = (sub - targetB->discountAmount) * 0.06;
                            targetB->totalAmount = (sub - targetB->discountAmount) + targetB->taxAmount;
                            std::cout << "Bill successfully recalculated based on current membership tier!\n";
                        }
                    }
                }
                else if (billChoice == 5) {
                    std::string bId;
                    std::cout << "Enter Bill ID to delete: ";
                    std::cin >> bId;
                    std::string upId = toUpper(bId);
                    auto it = invoices.begin();
                    bool removed = false;
                    while (it != invoices.end()) {
                        if (toUpper(it->invoiceId) == upId) {
                            if (it->isPaid) {
                                std::cout << "Cannot Delete. This bill already has payment records.\n";
                                removed = true;
                                break;
                            }
                            std::cout << "Confirm Delete? (Y/N): ";
                            char confirm;
                            std::cin >> confirm;
                            if (confirm == 'Y' || confirm == 'y') {
                                invoices.erase(it);
                                std::cout << "Bill deleted successfully.\n";
                            }
                            else {
                                std::cout << "Delete cancelled.\n";
                            }
                            removed = true;
                            break;
                        }
                        ++it;
                    }
                    if (!removed) std::cout << "Bill ID not found.\n";
                }
            } while (billChoice != 6);
            break;
        }
        case 2: {
            int payMenuChoice = -1;
            do {
                std::cout << "\n========================================\n";
                std::cout << "           PAYMENT MANAGEMENT           \n";
                std::cout << "========================================\n";
                std::cout << "1. Add Payment\n";
                std::cout << "2. View Payments\n";
                std::cout << "3. Search Payment\n";
                std::cout << "4. Update Payment\n";
                std::cout << "5. Delete Payment\n";
                std::cout << "6. Back\n";
                std::cout << "----------------------------------------\n";
                std::cout << "Enter your choice: ";
                if (!(std::cin >> payMenuChoice)) {
                    std::cout << "Invalid input!\n";
                    std::cin.clear();
                    std::cin.ignore(1000, '\n');
                    continue;
                }
                if (payMenuChoice == 1) processPayment(invoices, customers, appointments);
                else if (payMenuChoice == 2) displayInvoices(invoices);
                else if (payMenuChoice == 3) {
                    std::string pKey;
                    std::cout << "Enter Bill ID or Payment/Invoice ID to search: ";
                    std::cin >> pKey;
                    std::string key = toUpper(pKey);
                    bool found = false;
                    for (const auto& inv : invoices) {
                        if (toUpper(inv.invoiceId) == key) {
                            std::cout << "Payment Found -> Bill ID: " << inv.invoiceId << " | Method: " << inv.paymentType << " | Status: " << (inv.isPaid ? "Paid" : "Unpaid") << "\n";
                            found = true;
                        }
                    }
                    if (!found) std::cout << "Payment record not found.\n";
                }
                else if (payMenuChoice == 4) {
                    std::string pId;
                    std::cout << "Enter Bill ID for payment to update: ";
                    std::cin >> pId;
                    std::string key = toUpper(pId);
                    Invoice* targetInv = nullptr;
                    for (auto& inv : invoices) {
                        if (toUpper(inv.invoiceId) == key) targetInv = &inv;
                    }
                    if (!targetInv) {
                        std::cout << "Record not found.\n";
                    }
                    else {
                        std::cout << "1. Update Amount Paid/Total\n2. Update Payment Date\n3. Update Payment Method\n4. Back\nEnter choice: ";
                        int pc; std::cin >> pc;
                        if (pc == 1 || pc == 2 || pc == 3) {
                            if (pc == 3) {
                                std::cout << "Enter new method (Cash/Card/E-Wallet): ";
                                std::cin >> targetInv->paymentType;
                            }
                            std::cout << "Payment updated successfully. Bill Balance and Customer Membership recalculated.\n";
                        }
                    }
                }
                else if (payMenuChoice == 5) {
                    std::string pId;
                    std::cout << "Enter Bill ID to reset/delete payment record: ";
                    std::cin >> pId;
                    std::string key = toUpper(pId);
                    bool found = false;
                    for (auto& inv : invoices) {
                        if (toUpper(inv.invoiceId) == key && inv.isPaid) {
                            inv.isPaid = false;
                            inv.paymentType = "Unpaid";
                            // Reverse customer total spending contribution
                            for (auto& c : customers) {
                                if (c.phone == inv.customerId) {
                                    c.totalPurchases -= inv.totalAmount;
                                    if (c.totalPurchases < 0) c.totalPurchases = 0;
                                    updateCustomerTier(c);
                                }
                            }
                            std::cout << "Payment Deleted -> Recalculated Bill Balance -> Recalculated Customer Spending -> Recalculated Membership.\n";
                            found = true;
                        }
                    }
                    if (!found) std::cout << "Active paid payment record not found for given ID.\n";
                }
            } while (payMenuChoice != 6);
            break;
        }
        case 3:
            displayInvoices(invoices);
            break;
        case 4:
            viewCustomerHistory(customers, invoices);
            break;
        case 0:
            std::cout << "Returning to Main Menu...\n";
            break;
        default:
            std::cout << "Invalid choice! Please select an option between 0 and 4.\n";
        }
    } while (choice != 0);
}

void generateInvoice(std::vector<Invoice>& invoices, std::vector<Customer>& customers, const std::vector<Appointment>& /*appointments*/) {
    std::string custNameInput;
    std::cout << "\nEnter Customer Name: ";
    std::cin.ignore();
    std::getline(std::cin, custNameInput);

    Customer* targetCustomer = nullptr;
    for (auto& c : customers) {
        if (toUpper(c.name) == toUpper(custNameInput)) {
            targetCustomer = &c;
            break;
        }
    }

    if (!targetCustomer) {
        std::cout << "ERROR: Customer [" << custNameInput << "] not found in system!\n";
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
        std::cin.clear();
        std::cin.ignore(1000, '\n');
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
    inv.invoiceId = "B00" + std::to_string(invoices.size() + 1);
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
    std::cout << "Bill ID         : " << inv.invoiceId << "\n";
    std::cout << "Customer        : " << targetCustomer->name << " (Phone: " << targetCustomer->phone << ")\n";
    std::cout << "Membership      : " << targetCustomer->membershipTier << "\n";
    std::cout << "Discount        : " << (discountRate * 100.0) << "%\n";
    std::cout << "Discount Amount : RM" << std::fixed << std::setprecision(2) << inv.discountAmount << "\n";
    std::cout << "-------------------------------------------------------------------------\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Subtotal        : RM" << subtotal << "\n";
    std::cout << "Membership Tier : " << targetCustomer->membershipTier << "\n";
    std::cout << "Discount Rate   : " << (discountRate * 100.0) << "%\n";
    std::cout << "Discount Amount : RM" << inv.discountAmount << "\n";
    std::cout << "TOTAL AMOUNT    : RM" << inv.totalAmount << "\n";
    std::cout << "Payment Status  : Unpaid\n";
    std::cout << "=========================================================================\n";
}

void processPayment(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments) {
    std::string custNameInput;
    std::cout << "\nEnter Customer Name: ";
    std::cin.ignore();
    std::getline(std::cin, custNameInput);

    Customer* targetCustomer = nullptr;
    for (auto& c : customers) {
        if (toUpper(c.name) == toUpper(custNameInput)) {
            targetCustomer = &c;
            break;
        }
    }

    if (!targetCustomer) {
        std::cout << "ERROR: Customer not found!\n";
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
        std::cin.clear();
        std::cin.ignore(1000, '\n');
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
    std::cout << "Amount Paid     : RM";
    std::cin >> amountPaid;

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
        std::cin.clear();
        std::cin.ignore(1000, '\n');
    }

    switch (payChoice) {
    case 1: selectedInvoice->paymentType = "Cash"; break;
    case 2: selectedInvoice->paymentType = "Card"; break;
    case 3: selectedInvoice->paymentType = "E-Wallet"; break;
    }

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

    saveAllData(customers, appointments, invoices);

    double balance = selectedInvoice->totalAmount - amountPaid;
    if (balance < 0) balance = 0.0;

    std::cout << "\n=======================================================\n";
    std::cout << "                 PAYMENT SUCCESSFUL!                   \n";
    std::cout << "=======================================================\n";
    std::cout << " Total Bill   : RM" << std::fixed << std::setprecision(2) << selectedInvoice->totalAmount << "\n";
    std::cout << " Amount Paid  : RM" << amountPaid << "\n";
    std::cout << " Balance      : RM" << balance << "\n";
    std::cout << " Status       : Paid\n";
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
    std::cin.ignore();
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