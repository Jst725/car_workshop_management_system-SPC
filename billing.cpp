#include "billing.h"
#include "appointment.h"
#include "vehicle.h"
#include "customer.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>

// Helper: Converts any string to UPPERCASE (small letters to capital letters)
static std::string toUpper(std::string str) {
    for (char& c : str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return str;
}

// Helper: Get tier discount rate
static double getTierDiscountRate(const std::string& tier) {
    if (tier == "Silver") return 0.05;  // 5% discount
    if (tier == "Gold")   return 0.10;  // 10% discount
    if (tier == "VIP")    return 0.20;  // 20% discount
    return 0.0;                         // Standard: 0% discount
}

// Helper: Auto-update customer membership tier based on lifetime total purchases
static void updateCustomerTier(Customer& customer) {
    if (customer.totalPurchases >= 5000.0) customer.membershipTier = "VIP";
    else if (customer.totalPurchases >= 2000.0) customer.membershipTier = "Gold";
    else if (customer.totalPurchases >= 800.0)  customer.membershipTier = "Silver";
    else customer.membershipTier = "Standard";
}

void billingMenu(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "          BILLING & PAYMENT SYSTEM      \n";
        std::cout << "========================================\n";
        std::cout << "1. Generate Invoice from Vehicle Service\n";
        std::cout << "2. Process Payment for Invoice\n";
        std::cout << "3. Apply Custom Discount to Invoice   \n";
        std::cout << "4. View All Invoices\n";
        std::cout << "5. View Customer Billing & Service History\n";
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
            generateInvoice(invoices, customers, appointments);
            break;
        case 2:
            processPayment(invoices, customers, appointments);
            break;
        case 3:
            applyCustomDiscount(invoices);
            break;
        case 4:
            displayInvoices(invoices);
            break;
        case 5:
            viewCustomerHistory(customers, invoices);
            break;
        case 0:
            std::cout << "Returning to Main Menu...\n";
            break;
        default:
            std::cout << "Invalid choice! Please select an option between 0 and 5.\n";
        }
    } while (choice != 0);
}

void generateInvoice(std::vector<Invoice>& invoices, std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::string plateInput;
    std::cout << "\nEnter Vehicle License Plate (or 0 to Return): ";
    std::cin >> plateInput;
    if (plateInput == "0") return;

    std::string plate = toUpper(plateInput);

    for (auto& c : customers) {
        for (auto& v : c.vehicles) {
            if (v.licensePlate == plate) {

                if (v.currentServiceCost <= 0.0) {
                    std::cout << "ERROR: Vehicle [" << plate << "] has no accrued service charges to bill!\n";
                    return;
                }

                if (v.status != "Ready for Pickup") {
                    std::cout << "\n=======================================================\n";
                    std::cout << " CANNOT GENERATE INVOICE YET                           \n";
                    std::cout << " Current Status: [" << v.status << "]\n";
                    std::cout << " Service process must be finished ('Ready for Pickup') \n";
                    std::cout << " before an invoice can be finalized for payment.       \n";
                    std::cout << "=======================================================\n";
                    return;
                }

                for (const auto& existingInv : invoices) {
                    if (existingInv.licensePlate == plate && !existingInv.isPaid) {
                        std::cout << "INFO: An active pending invoice [" << existingInv.invoiceId << "] already exists for " << plate << "!\n";
                        return;
                    }
                }

                Invoice inv;
                inv.invoiceId = "INV" + std::to_string(invoices.size() + 101);
                inv.customerId = c.id;
                inv.licensePlate = v.licensePlate;
                inv.date = v.lastServiceDate;
                inv.partsCost = v.partsCost;
                inv.laborCost = v.laborCost;

                double subtotal = inv.partsCost + inv.laborCost;
                double discountRate = getTierDiscountRate(c.membershipTier);
                inv.discountAmount = subtotal * discountRate;

                double taxableAmount = subtotal - inv.discountAmount;
                inv.taxAmount = taxableAmount * 0.06; // 6% Service Tax
                inv.totalAmount = taxableAmount + inv.taxAmount;
                inv.isPaid = false;
                inv.paymentType = "Pending";

                invoices.push_back(inv);
                saveInvoicesToFile(invoices);

                std::cout << "\n=======================================================\n";
                std::cout << "               OFFICIAL WORKSHOP INVOICE               \n";
                std::cout << "=======================================================\n";
                std::cout << "Invoice ID    : " << inv.invoiceId << "\n";
                std::cout << "Date          : " << inv.date << "\n";
                std::cout << "Customer ID   : " << c.id << " (" << c.name << " | " << c.phone << ")\n";
                std::cout << "Vehicle Plate : " << v.licensePlate << " (" << v.brand << " " << v.model << ")\n";
                std::cout << "Technician    : " << v.assignedTechnician << "\n";
                std::cout << "-------------------------------------------------------\n";
                std::cout << "ITEMIZED REPLACEMENT PARTS BREAKDOWN:\n";

                if (v.parts.empty()) {
                    std::cout << "  (No parts replacement required)\n";
                }
                else {
                    for (size_t i = 0; i < v.parts.size(); ++i) {
                        std::cout << "  " << (i + 1) << ". " << std::left << std::setw(30) << v.parts[i].name
                            << " $" << std::fixed << std::setprecision(2) << v.parts[i].price << "\n";
                    }
                }

                std::cout << "-------------------------------------------------------\n";
                std::cout << std::fixed << std::setprecision(2);
                std::cout << "Parts Subtotal  : $" << inv.partsCost << "\n";
                std::cout << "Labor Subtotal  : $" << inv.laborCost << "\n";
                std::cout << "Gross Subtotal  : $" << subtotal << "\n";
                std::cout << "Tier Discount   : -$" << inv.discountAmount << " (" << c.membershipTier << " Tier)\n";
                std::cout << "Service Tax (6%): $" << inv.taxAmount << "\n";
                std::cout << "-------------------------------------------------------\n";
                std::cout << "NET TOTAL DUE   : $" << inv.totalAmount << "\n";
                std::cout << "Payment Status  : PENDING PAYMENT\n";
                std::cout << "=======================================================\n";
                return;
            }
        }
    }
    std::cout << "ERROR: Vehicle license plate [" << plate << "] not found in system!\n";
}

void processPayment(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments) {
    std::string invIdInput;
    std::cout << "\nEnter Invoice ID to Pay (e.g. inv101) or 0 to Return: ";
    std::cin >> invIdInput;
    if (invIdInput == "0") return;

    std::string invId = toUpper(invIdInput);

    Invoice* selectedInvoice = nullptr;
    for (auto& inv : invoices) {
        if (inv.invoiceId == invId) {
            selectedInvoice = &inv;
            break;
        }
    }

    if (!selectedInvoice) {
        std::cout << "ERROR: Invoice ID [" << invId << "] not found!\n";
        return;
    }

    if (selectedInvoice->isPaid) {
        std::cout << "INFO: Invoice [" << invId << "] has ALREADY been paid!\n";
        return;
    }

    Customer* targetCustomer = nullptr;
    for (auto& c : customers) {
        if (c.id == selectedInvoice->customerId) {
            targetCustomer = &c;
            break;
        }
    }

    if (!targetCustomer) {
        std::cout << "ERROR: Customer record associated with invoice not found!\n";
        return;
    }

    Vehicle* targetVehicle = nullptr;
    for (auto& v : targetCustomer->vehicles) {
        if (v.licensePlate == selectedInvoice->licensePlate) {
            targetVehicle = &v;
            break;
        }
    }

    if (targetVehicle && targetVehicle->status != "Ready for Pickup") {
        std::cout << "ERROR: Workshop process not completed! Status is '" << targetVehicle->status << "'. Must be 'Ready for Pickup' before paying.\n";
        return;
    }

    std::cout << "\n=======================================================\n";
    std::cout << "             INVOICE PAYMENT PROCESSING                \n";
    std::cout << "=======================================================\n";
    std::cout << "Invoice ID     : " << selectedInvoice->invoiceId << "\n";
    std::cout << "Customer Name  : " << targetCustomer->name << " (" << targetCustomer->membershipTier << " Tier)\n";
    std::cout << "Vehicle Plate  : " << selectedInvoice->licensePlate << "\n";
    std::cout << "Parts Subtotal : $" << std::fixed << std::setprecision(2) << selectedInvoice->partsCost << "\n";
    std::cout << "Labor Subtotal : $" << selectedInvoice->laborCost << "\n";
    std::cout << "Discount Saved : $" << selectedInvoice->discountAmount << "\n";
    std::cout << "Tax Amount     : $" << selectedInvoice->taxAmount << "\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << "NET AMOUNT DUE : $" << selectedInvoice->totalAmount << "\n";
    std::cout << "=======================================================\n";

    int payChoice = 1;
    while (true) {
        std::cout << "Select Payment Method:\n";
        std::cout << "1. Cash\n2. Card\n3. E-Wallet\nChoice: ";
        if (std::cin >> payChoice && payChoice >= 1 && payChoice <= 3) {
            break;
        }
        std::cout << "Invalid choice! Please select 1, 2, or 3.\n";
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

    std::cout << "\n=======================================================\n";
    std::cout << "               PAYMENT SUCCESSFUL!                     \n";
    std::cout << "=======================================================\n";
    std::cout << " Invoice ID       : " << selectedInvoice->invoiceId << "\n";
    std::cout << " Payment Method   : " << selectedInvoice->paymentType << "\n";
    std::cout << " Amount Paid      : $" << std::fixed << std::setprecision(2) << selectedInvoice->totalAmount << "\n";
    std::cout << " Lifetime Spent   : $" << targetCustomer->totalPurchases << "\n";
    std::cout << " Current Tier     : " << targetCustomer->membershipTier << "\n";
    std::cout << " Vehicle Status   : CLEARED & READY FOR PICKUP\n";
    std::cout << "=======================================================\n";
}

void applyCustomDiscount(std::vector<Invoice>& invoices) {
    std::string invIdInput;
    std::cout << "\nEnter Invoice ID for Custom Discount (or 0 to Return): ";
    std::cin >> invIdInput;
    if (invIdInput == "0") return;

    std::string invId = toUpper(invIdInput);
    Invoice* targetInvoice = nullptr;

    for (auto& inv : invoices) {
        if (inv.invoiceId == invId) {
            targetInvoice = &inv;
            break;
        }
    }

    if (!targetInvoice) {
        std::cout << "ERROR: Invoice ID [" << invId << "] not found!\n";
        return;
    }

    if (targetInvoice->isPaid) {
        std::cout << "ERROR: Cannot modify discounts on an invoice that is already PAID!\n";
        return;
    }

    int discountType = 1;
    while (true) {
        std::cout << "\nSelect Discount Type:\n";
        std::cout << "1. Fixed Amount ($)\n";
        std::cout << "2. Percentage (%)\n";
        std::cout << "Choice: ";
        if (std::cin >> discountType && (discountType == 1 || discountType == 2)) {
            break;
        }
        std::cout << "Invalid choice! Enter 1 or 2.\n";
        std::cin.clear();
        std::cin.ignore(1000, '\n');
    }

    double subtotal = targetInvoice->partsCost + targetInvoice->laborCost;
    double customDiscountValue = 0.0;

    if (discountType == 1) {
        while (true) {
            std::cout << "Enter Fixed Discount Amount ($): $";
            if (std::cin >> customDiscountValue && customDiscountValue >= 0.0) {
                break;
            }
            std::cout << "Invalid amount! Enter a positive value.\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
        }
    }
    else {
        double percent = 0.0;
        while (true) {
            std::cout << "Enter Discount Percentage (0 - 100%): ";
            if (std::cin >> percent && percent >= 0.0 && percent <= 100.0) {
                break;
            }
            std::cout << "Invalid percentage! Enter a value between 0 and 100.\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
        }
        customDiscountValue = subtotal * (percent / 100.0);
    }

    if (customDiscountValue > subtotal) {
        customDiscountValue = subtotal;
    }

    targetInvoice->discountAmount += customDiscountValue;
    double taxableAmount = subtotal - targetInvoice->discountAmount;
    targetInvoice->taxAmount = taxableAmount * 0.06;
    targetInvoice->totalAmount = taxableAmount + targetInvoice->taxAmount;

    saveInvoicesToFile(invoices);

    std::cout << "\n=======================================================\n";
    std::cout << "               CUSTOM DISCOUNT APPLIED                 \n";
    std::cout << "=======================================================\n";
    std::cout << " Invoice ID     : " << targetInvoice->invoiceId << "\n";
    std::cout << " Discount Added : $" << std::fixed << std::setprecision(2) << customDiscountValue << "\n";
    std::cout << " NEW TOTAL DUE  : $" << targetInvoice->totalAmount << "\n";
    std::cout << "=======================================================\n";
}

void displayInvoices(const std::vector<Invoice>& invoices) {
    if (invoices.empty()) {
        std::cout << "\nNo invoice records found in storage.\n";
        return;
    }

    std::cout << "\n---------------------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(10) << "Inv ID"
        << std::setw(10) << "Cust ID"
        << std::setw(12) << "Plate"
        << std::setw(12) << "Parts ($)"
        << std::setw(12) << "Labor ($)"
        << std::setw(12) << "Total ($)"
        << std::setw(12) << "Method"
        << "Status\n";
    std::cout << "---------------------------------------------------------------------------------------------------\n";

    for (const auto& inv : invoices) {
        std::cout << std::left << std::setw(10) << inv.invoiceId
            << std::setw(10) << inv.customerId
            << std::setw(12) << inv.licensePlate
            << std::setw(12) << std::fixed << std::setprecision(2) << inv.partsCost
            << std::setw(12) << inv.laborCost
            << std::setw(12) << inv.totalAmount
            << std::setw(12) << inv.paymentType
            << (inv.isPaid ? "PAID" : "PENDING") << "\n";
    }
}

void viewCustomerHistory(const std::vector<Customer>& customers, const std::vector<Invoice>& invoices) {
    std::string input;
    std::cout << "\nEnter Customer ID or License Plate (e.g. c101 or bje447) or 0 to Return: ";
    std::cin >> input;
    if (input == "0") return;

    std::string searchKey = toUpper(input);
    const Customer* foundCustomer = nullptr;

    for (const auto& c : customers) {
        if (toUpper(c.id) == searchKey) {
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
    std::cout << "Customer ID    : " << foundCustomer->id << "\n";
    std::cout << "Name           : " << foundCustomer->name << "\n";
    std::cout << "Phone          : " << foundCustomer->phone << "\n";
    std::cout << "Membership Tier: " << foundCustomer->membershipTier << "\n";
    std::cout << "Lifetime Spent : $" << std::fixed << std::setprecision(2) << foundCustomer->totalPurchases << "\n";
    std::cout << "-------------------------------------------------------\n";

    std::cout << "REGISTERED VEHICLES:\n";
    if (foundCustomer->vehicles.empty()) {
        std::cout << "  (No vehicles registered)\n";
    }
    else {
        for (size_t i = 0; i < foundCustomer->vehicles.size(); ++i) {
            const auto& v = foundCustomer->vehicles[i];
            std::cout << "  " << (i + 1) << ". " << v.licensePlate
                << " (" << v.brand << " " << v.model << ") | Last Service: " << v.lastServiceDate << "\n";
        }
    }

    std::cout << "-------------------------------------------------------\n";
    std::cout << "TRANSACTION & INVOICE HISTORY:\n";

    bool hasInvoices = false;
    std::cout << std::left << std::setw(10) << "Inv ID"
        << std::setw(12) << "Plate"
        << std::setw(12) << "Date"
        << std::setw(12) << "Total ($)"
        << std::setw(12) << "Method"
        << "Status\n";
    std::cout << "-------------------------------------------------------\n";

    for (const auto& inv : invoices) {
        if (inv.customerId == foundCustomer->id) {
            hasInvoices = true;
            std::cout << std::left << std::setw(10) << inv.invoiceId
                << std::setw(12) << inv.licensePlate
                << std::setw(12) << inv.date
                << std::setw(12) << std::fixed << std::setprecision(2) << inv.totalAmount
                << std::setw(12) << inv.paymentType
                << (inv.isPaid ? "PAID" : "PENDING") << "\n";
        }
    }

    if (!hasInvoices) {
        std::cout << "No past invoice records found for this customer.\n";
    }
    std::cout << "=======================================================\n";
}