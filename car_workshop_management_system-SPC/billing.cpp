#include "billing.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <string>

void billingMenu(std::vector<Invoice>& invoices, std::vector<Customer>& customers) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "         BILLING & PAYMENT MANAGEMENT   \n";
        std::cout << "========================================\n";
        std::cout << "1. Generate Invoice\n";
        std::cout << "2. Process Payment & Apply Loyalty Discount\n";
        std::cout << "3. Customer Payment History\n";
        std::cout << "0. Return to Main Menu\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Enter choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input!\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        switch (choice) {
        case 1: generateInvoice(invoices, customers); saveInvoicesToFile(invoices); break;
        case 2: processPayment(invoices, customers); saveInvoicesToFile(invoices); break;
        case 3: paymentHistory(invoices); break;
        case 0: std::cout << "Returning to Main Menu...\n"; break;
        default: std::cout << "Invalid choice!\n";
        }
    } while (choice != 0);
}

void generateInvoice(std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    std::string custId, plate;
    std::cout << "\nEnter Customer ID (or 0 to Return to Main Menu): ";
    std::cin >> custId;
    if (custId == "0") return;

    std::cout << "Enter Vehicle License Plate: ";
    std::cin >> plate;

    double activeCost = 0.0;
    bool found = false;

    for (const auto& c : customers) {
        if (c.id == custId) {
            for (const auto& v : c.vehicles) {
                if (v.licensePlate == plate) {
                    activeCost = v.currentServiceCost;
                    found = true;
                    break;
                }
            }
        }
    }

    if (!found) {
        std::cout << "ERROR: Customer or Plate match not found!\n";
        return;
    }

    Invoice inv;
    inv.invoiceId = "INV" + std::to_string(invoices.size() + 5001);
    inv.customerId = custId;
    inv.licensePlate = plate;
    inv.date = "2026-08-02";
    inv.partsCost = activeCost * 0.65;
    inv.laborCost = activeCost * 0.35;
    inv.isPaid = false;

    invoices.push_back(inv);
    std::cout << "\nInvoice " << inv.invoiceId << " generated successfully!\n";
}

void processPayment(std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    std::string invId;
    std::cout << "\nEnter Invoice ID to process payment (or 0 to Return to Main Menu): ";
    std::cin >> invId;
    if (invId == "0") return;

    for (auto& inv : invoices) {
        if (inv.invoiceId == invId && !inv.isPaid) {
            std::string tier = "Standard";
            for (const auto& c : customers) {
                if (c.id == inv.customerId) {
                    tier = c.membershipTier;
                    break;
                }
            }

            double subtotal = inv.partsCost + inv.laborCost;
            double discountRate = 0.0;

            if (tier == "Gold") discountRate = 0.15;
            else if (tier == "Silver") discountRate = 0.10;

            inv.discountAmount = subtotal * discountRate;
            double discountedSubtotal = subtotal - inv.discountAmount;
            inv.taxAmount = discountedSubtotal * 0.06;
            inv.totalAmount = discountedSubtotal + inv.taxAmount;

            std::cout << "\nSelect Payment Method:\n";
            std::cout << "1. Cash\n";
            std::cout << "2. Card\n";
            std::cout << "3. E-Wallet\n";
            std::cout << "0. Return to Main Menu\n";
            std::cout << "Choice: ";
            int pChoice;
            std::cin >> pChoice;
            if (pChoice == 0) return;
            if (pChoice == 1) inv.paymentType = "Cash";
            else if (pChoice == 2) inv.paymentType = "Card";
            else inv.paymentType = "E-Wallet";

            inv.isPaid = true;
            std::cout << "\nPayment Processed Successfully!\n";
            printReceipt(inv);
            return;
        }
    }
    std::cout << "Invoice ID not found or already paid.\n";
}

void printReceipt(const Invoice& inv) {
    std::cout << "\n========================================\n";
    std::cout << "          OFFICIAL RECEIPT              \n";
    std::cout << "========================================\n";
    std::cout << "Invoice ID    : " << inv.invoiceId << "\n";
    std::cout << "Customer ID   : " << inv.customerId << "\n";
    std::cout << "Vehicle Plate : " << inv.licensePlate << "\n";
    std::cout << "Date          : " << inv.date << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Parts Charge  : $" << inv.partsCost << "\n";
    std::cout << "Labor Charge  : $" << inv.laborCost << "\n";
    std::cout << "Discount      : -$" << inv.discountAmount << "\n";
    std::cout << "Tax (6%)      : $" << inv.taxAmount << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "TOTAL PAID    : $" << inv.totalAmount << "\n";
    std::cout << "Payment Type  : " << inv.paymentType << "\n";
    std::cout << "========================================\n";
}

void paymentHistory(const std::vector<Invoice>& invoices) {
    std::string custId;
    std::cout << "\nEnter Customer ID for Payment History (or 0 to Return to Main Menu): ";
    std::cin >> custId;
    if (custId == "0") return;

    bool found = false;
    for (const auto& inv : invoices) {
        if (inv.customerId == custId) {
            std::cout << "Invoice: " << std::setw(10) << inv.invoiceId
                << " | Date: " << inv.date
                << " | Total: $" << std::fixed << std::setprecision(2) << inv.totalAmount
                << " | Paid: " << (inv.isPaid ? "YES" : "NO") << "\n";
            found = true;
        }
    }
    if (!found) std::cout << "No payment history found for this customer.\n";
}