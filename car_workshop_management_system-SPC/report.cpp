#include "report.h"
#include <iostream>
#include <fstream>
#include <iomanip>

void reportMenu(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "         REPORTS & ANALYTICS            \n";
        std::cout << "========================================\n";
        std::cout << "1. Daily Revenue Report\n";
        std::cout << "2. Customer Statistics\n";
        std::cout << "3. Export Reports to Text File\n";
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
        case 1: dailyReport(invoices); break;
        case 2: customerStatistics(customers); break;
        case 3: exportReport(invoices); break;
        case 0: std::cout << "Returning to Main Menu...\n"; break;
        default: std::cout << "Invalid choice!\n";
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
    int statsMatrix[3][2] = { 0 };

    for (const auto& c : customers) {
        int rowIdx = 0;
        if (c.membershipTier == "Silver") rowIdx = 1;
        else if (c.membershipTier == "Gold") rowIdx = 2;

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
            outFile << "Invoice: " << inv.invoiceId
                << " | Cust ID: " << inv.customerId
                << " | Date: " << inv.date
                << " | Amount: $" << inv.totalAmount << "\n";
        }
    }
    outFile.close();
    std::cout << "Report successfully exported to 'Workshop_Financial_Report.txt'!\n";
}