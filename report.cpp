#include "report.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

// Helper: Converts any string to UPPERCASE for comparisons
static std::string toUpper(std::string str) {
    for (char& c : str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return str;
}

void reportMenu(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "         REPORTS & ANALYTICS            \n";
        std::cout << "========================================\n";
        std::cout << "1. Daily Revenue Report\n";
        std::cout << "2. Customer Statistics Matrix\n";
        std::cout << "3. Top Spending Customers Analytics   \n"; // <--- NEW MENU ITEM
        std::cout << "4. Export Reports to Text File\n";
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
        case 1: dailyReport(invoices); break;
        case 2: customerStatistics(customers); break;
        case 3: topSpendingCustomers(customers); break; // <--- NEW CASE HANDLER
        case 4: exportReport(invoices); break;
        case 0: std::cout << "Returning to Main Menu...\n"; break;
        default: std::cout << "Invalid choice! Please select an option between 0 and 4.\n";
        }
    } while (choice != 0);
}

void dailyReport(const std::vector<Invoice>& invoices) {
    double totalRev = 0.0, totalTax = 0.0;
    int count = 0;
    for (const auto& inv : invoices) {
        if (inv.isPaid) {
            totalRev += inv.totalAmount;
            totalTax += inv.taxAmount;
            count++;
        }
    }
    std::cout << "\n--- WORKSHOP FINANCIAL REPORT ---\n";
    std::cout << "Transactions Processed: " << count << "\n";
    std::cout << "Total Gross Revenue   : $" << std::fixed << std::setprecision(2) << totalRev << "\n";
    std::cout << "Total Tax Collected   : $" << totalTax << "\n";
}

void customerStatistics(const std::vector<Customer>& customers) {
    int statsMatrix[4][2] = { {0} }; // Rows for Standard, Silver, Gold, VIP

    for (const auto& c : customers) {
        int rowIdx = 0;
        if (c.membershipTier == "Silver") rowIdx = 1;
        else if (c.membershipTier == "Gold") rowIdx = 2;
        else if (c.membershipTier == "VIP") rowIdx = 3;

        statsMatrix[rowIdx][0]++;
        statsMatrix[rowIdx][1] += static_cast<int>(c.vehicles.size());
    }

    std::cout << "\n--- CUSTOMER TIER & VEHICLE STATISTICS MATRIX ---\n";
    std::cout << std::left << std::setw(15) << "Tier"
        << std::setw(18) << "Total Customers"
        << "Total Cars\n";
    std::cout << "---------------------------------------------------------\n";
    std::cout << std::left << std::setw(15) << "Standard" << std::setw(18) << statsMatrix[0][0] << statsMatrix[0][1] << "\n";
    std::cout << std::left << std::setw(15) << "Silver" << std::setw(18) << statsMatrix[1][0] << statsMatrix[1][1] << "\n";
    std::cout << std::left << std::setw(15) << "Gold" << std::setw(18) << statsMatrix[2][0] << statsMatrix[2][1] << "\n";
    std::cout << std::left << std::setw(15) << "VIP" << std::setw(18) << statsMatrix[3][0] << statsMatrix[3][1] << "\n";
}

// NEW FUNCTION: Analyzes and displays top spending customers
void topSpendingCustomers(const std::vector<Customer>& customers) {
    if (customers.empty()) {
        std::cout << "\nNo customer records found for analysis.\n";
        return;
    }

    // Create a copy to sort without mutating original vector order
    std::vector<Customer> sortedCustomers = customers;
    std::sort(sortedCustomers.begin(), sortedCustomers.end(), [](const Customer& a, const Customer& b) {
        return a.totalPurchases > b.totalPurchases;
        });

    std::cout << "\n=========================================================================\n";
    std::cout << "                  TOP SPENDING CUSTOMERS LEADERBOARD                     \n";
    std::cout << "=========================================================================\n";
    std::cout << std::left << std::setw(8) << "Rank"
        << std::setw(10) << "Cust ID"
        << std::setw(22) << "Name"
        << std::setw(15) << "Tier"
        << "Lifetime Spent ($)\n";
    std::cout << "-------------------------------------------------------------------------\n";

    size_t limit = std::min(size_t(5), sortedCustomers.size());
    for (size_t i = 0; i < limit; ++i) {
        std::cout << std::left << std::setw(8) << (i + 1)
            << std::setw(10) << sortedCustomers[i].id
            << std::setw(22) << sortedCustomers[i].name
            << std::setw(15) << sortedCustomers[i].membershipTier
            << "$" << std::fixed << std::setprecision(2) << sortedCustomers[i].totalPurchases << "\n";
    }
    std::cout << "=========================================================================\n";
}

void exportReport(const std::vector<Invoice>& invoices) {
    std::ofstream outFile("Workshop_Financial_Report.txt");
    if (!outFile) {
        std::cout << "ERROR: Failed to create export file!\n";
        return;
    }

    outFile << "====================================================\n";
    outFile << "    WORKSHOP MANAGEMENT FINANCIAL REPORT EXPORT     \n";
    outFile << "====================================================\n";
    for (const auto& inv : invoices) {
        if (inv.isPaid) {
            outFile << "Invoice: " << std::left << std::setw(10) << inv.invoiceId
                << " | Cust ID: " << std::left << std::setw(8) << inv.customerId
                << " | Date: " << std::left << std::setw(12) << inv.date
                << " | Amount: $" << std::fixed << std::setprecision(2) << inv.totalAmount << "\n";
        }
    }
    outFile.close();
    std::cout << "Report successfully exported to 'Workshop_Financial_Report.txt'!\n";
}