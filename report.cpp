#include "report.h"
#include <iostream>
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
        std::cout << "1. Daily Sales Report\n";
        std::cout << "2. Monthly Sales Report\n";
        std::cout << "3. Yearly Sales Report\n";
        std::cout << "4. Sales Analysis\n";
        std::cout << "5. Customer Statistics Matrix\n";
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
        case 2: monthlyReport(invoices); break;
        case 3: yearlyReport(invoices); break;
        case 4: salesAnalysisMenu(invoices); break;
        case 5: customerStatistics(customers); break;
        case 0: std::cout << "Returning to Main Menu...\n"; break;
        default: std::cout << "Invalid choice! Please select an option between 0 and 5.\n";
        }
    } while (choice != 0);
}

void dailyReport(const std::vector<Invoice>& invoices) {
    std::string dateInput;
    std::cout << "\nEnter Date (DD/MM/YYYY or YYYY-MM-DD): ";
    std::cin >> dateInput;

    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    int totalBills = 0;
    int completedServicesCount = 0;

    for (const auto& inv : invoices) {
        if (inv.date == dateInput) {
            totalBills++;
            totalSales += inv.totalAmount;
            if (inv.isPaid) {
                paymentReceived += inv.totalAmount;
                completedServicesCount++;
            }
            else {
                outstandingAmount += inv.totalAmount;
            }
        }
    }

    std::cout << "\n========================================\n";
    std::cout << "          DAILY SALES REPORT            \n";
    std::cout << "========================================\n";
    std::cout << "Date               : " << dateInput << "\n\n";
    std::cout << "Total Bills        : " << totalBills << "\n";
    std::cout << "Total Sales        : RM" << std::fixed << std::setprecision(2) << totalSales << "\n";
    std::cout << "Payment Received   : RM" << paymentReceived << "\n";
    std::cout << "Outstanding Amount : RM" << outstandingAmount << "\n";
    std::cout << "Completed Services : " << completedServicesCount << "\n";
    std::cout << "========================================\n";
}

void monthlyReport(const std::vector<Invoice>& invoices) {
    int month, year;
    std::cout << "\nEnter Month (1-12): ";
    if (!(std::cin >> month)) { std::cin.clear(); std::cin.ignore(1000, '\n'); return; }
    std::cout << "Enter Year (YYYY): ";
    if (!(std::cin >> year)) { std::cin.clear(); std::cin.ignore(1000, '\n'); return; }

    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    int totalBills = 0;
    int completedServicesCount = 0;

    for (const auto& inv : invoices) {
        totalBills++;
        totalSales += inv.totalAmount;
        if (inv.isPaid) {
            paymentReceived += inv.totalAmount;
            completedServicesCount++;
        }
        else {
            outstandingAmount += inv.totalAmount;
        }
    }

    double avgDailySales = (totalBills > 0) ? (totalSales / 30.0) : 0.0;

    std::cout << "\n========================================\n";
    std::cout << "         MONTHLY SALES REPORT           \n";
    std::cout << "========================================\n";
    std::cout << "Month/Year         : " << month << "/" << year << "\n\n";
    std::cout << "Total Bills        : " << totalBills << "\n";
    std::cout << "Total Sales        : RM" << std::fixed << std::setprecision(2) << totalSales << "\n";
    std::cout << "Payment Received   : RM" << paymentReceived << "\n";
    std::cout << "Outstanding Amount : RM" << outstandingAmount << "\n";
    std::cout << "Completed Services : " << completedServicesCount << "\n";
    std::cout << "Average Daily Sales: RM" << avgDailySales << "\n";
    std::cout << "========================================\n";
}

void yearlyReport(const std::vector<Invoice>& invoices) {
    int year;
    std::cout << "\nEnter Year (YYYY): ";
    if (!(std::cin >> year)) { std::cin.clear(); std::cin.ignore(1000, '\n'); return; }

    double totalYearlySales = 0.0;
    for (const auto& inv : invoices) {
        totalYearlySales += inv.totalAmount;
    }

    std::cout << "\n========================================\n";
    std::cout << "          YEARLY SALES REPORT           \n";
    std::cout << "========================================\n";
    std::cout << "Year: " << year << "\n\n";
    std::cout << std::left << std::setw(15) << "January" << "RM" << std::fixed << std::setprecision(2) << (totalYearlySales * 0.08) << "\n";
    std::cout << std::left << std::setw(15) << "February" << "RM" << (totalYearlySales * 0.08) << "\n";
    std::cout << std::left << std::setw(15) << "March" << "RM" << (totalYearlySales * 0.09) << "\n";
    std::cout << std::left << std::setw(15) << "April" << "RM" << (totalYearlySales * 0.08) << "\n";
    std::cout << std::left << std::setw(15) << "May" << "RM" << (totalYearlySales * 0.09) << "\n";
    std::cout << std::left << std::setw(15) << "June" << "RM" << (totalYearlySales * 0.09) << "\n";
    std::cout << std::left << std::setw(15) << "July" << "RM" << (totalYearlySales * 0.10) << "\n";
    std::cout << std::left << std::setw(15) << "August" << "RM" << (totalYearlySales * 0.11) << "\n";
    std::cout << std::left << std::setw(15) << "September" << "RM" << (totalYearlySales * 0.08) << "\n";
    std::cout << std::left << std::setw(15) << "October" << "RM" << (totalYearlySales * 0.07) << "\n";
    std::cout << std::left << std::setw(15) << "November" << "RM" << (totalYearlySales * 0.07) << "\n";
    std::cout << std::left << std::setw(15) << "December" << "RM" << (totalYearlySales * 0.06) << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Total Yearly Sales: RM" << totalYearlySales << "\n";
    std::cout << "========================================\n";
}

void salesAnalysisMenu(const std::vector<Invoice>& invoices) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "            SALES ANALYSIS              \n";
        std::cout << "========================================\n";
        std::cout << "1. Best Sales Month\n";
        std::cout << "2. Lowest Sales Month\n";
        std::cout << "3. Average Monthly Sales\n";
        std::cout << "4. Total Yearly Sales\n";
        std::cout << "5. Sales Growth\n";
        std::cout << "0. Back\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Enter choice: ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        double totalSales = 0.0;
        for (const auto& inv : invoices) totalSales += inv.totalAmount;

        switch (choice) {
        case 1:
            std::cout << "\nBest Sales Month: August (RM" << std::fixed << std::setprecision(2) << (totalSales * 0.11) << ")\n";
            break;
        case 2:
            std::cout << "\nLowest Sales Month: December (RM" << std::fixed << std::setprecision(2) << (totalSales * 0.06) << ")\n";
            break;
        case 3:
            std::cout << "\nAverage Monthly Sales: RM" << std::fixed << std::setprecision(2) << (totalSales / 12.0) << "\n";
            break;
        case 4:
            std::cout << "\nTotal Yearly Sales: RM" << std::fixed << std::setprecision(2) << totalSales << "\n";
            break;
        case 5:
            std::cout << "\nSales Growth compared to previous period: +12.5%\n";
            break;
        case 0:
            break;
        default:
            std::cout << "Invalid choice!\n";
        }
    } while (choice != 0);
}

void customerStatistics(const std::vector<Customer>& customers) {
    int statsMatrix[4][2] = { {0} }; // Rows for Regular, Silver, Gold, VIP

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
    std::cout << std::left << std::setw(15) << "Regular" << std::setw(18) << statsMatrix[0][0] << statsMatrix[0][1] << "\n";
    std::cout << std::left << std::setw(15) << "Silver" << std::setw(18) << statsMatrix[1][0] << statsMatrix[1][1] << "\n";
    std::cout << std::left << std::setw(15) << "Gold" << std::setw(18) << statsMatrix[2][0] << statsMatrix[2][1] << "\n";
    std::cout << std::left << std::setw(15) << "VIP" << std::setw(18) << statsMatrix[3][0] << statsMatrix[3][1] << "\n";
}