#include "report.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <array>
#include <limits>
#include <fstream>
#include <ctime>

// ---------------------------------------------------------------------------
// Robust Date Parsing Helper (using std::tm for strict validation)
// ---------------------------------------------------------------------------
struct ParsedDate {
    int day = 0;
    int month = 0;
    int year = 0;
};

static bool parseDate(const std::string& raw, ParsedDate& out) {
    if (raw.size() < 8) return false;

    int d = 0, m = 0, y = 0;
    char sep1 = '\0', sep2 = '\0';
    std::stringstream ss(raw);

    // Try DD/MM/YYYY
    if ((ss >> d >> sep1 >> m >> sep2 >> y) && sep1 == '/' && sep2 == '/') {
        std::tm time_in = {};
        time_in.tm_mday = d;
        time_in.tm_mon = m - 1;
        time_in.tm_year = y - 1900;
        time_in.tm_isdst = -1;

        std::mktime(&time_in);
        if (time_in.tm_mday == d && time_in.tm_mon == (m - 1) && time_in.tm_year == (y - 1900)) {
            out = { d, m, y };
            return true;
        }
    }

    // Try YYYY-MM-DD
    ss.clear();
    ss.str(raw);
    if ((ss >> y >> sep1 >> m >> sep2 >> d) && sep1 == '-' && sep2 == '-') {
        std::tm time_in = {};
        time_in.tm_mday = d;
        time_in.tm_mon = m - 1;
        time_in.tm_year = y - 1900;
        time_in.tm_isdst = -1;

        std::mktime(&time_in);
        if (time_in.tm_mday == d && time_in.tm_mon == (m - 1) && time_in.tm_year == (y - 1900)) {
            out = { d, m, y };
            return true;
        }
    }

    return false;
}

// Helper: Safely clear std::cin state upon bad input
static void clearCinError() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Helper: Get days in a month safely
static int daysInMonth(int month, int year) {
    static const int base[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2) {
        bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        return leap ? 29 : 28;
    }
    return (month >= 1 && month <= 12) ? base[month - 1] : 30;
}

// ---------------------------------------------------------------------------
// Report Menu Implementations
// ---------------------------------------------------------------------------

void reportMenu(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n"
            << "          REPORTS & ANALYTICS           \n"
            << "========================================\n"
            << "1. All Reports Summary (Daily, Monthly, Yearly)\n"
            << "2. Customer Statistics\n"
            << "3. Export Reports\n"
            << "0. Return to Main Menu\n"
            << "----------------------------------------\n"
            << "Enter choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input! Please enter a number.\n";
            clearCinError();
            continue;
        }

        switch (choice) {
        case 1: allReportsSummaryMenu(invoices); break;
        case 2: customerStatistics(customers); break;
        case 3: exportReports(invoices, customers); break;
        case 0: std::cout << "Returning to Main Menu...\n"; break;
        default: std::cout << "Invalid choice! Please select an option between 0 and 3.\n";
        }
    } while (choice != 0);
}

void allReportsSummaryMenu(const std::vector<Invoice>& invoices) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n"
            << "          ALL REPORTS SUMMARY           \n"
            << "========================================\n"
            << "1. Daily Sales Report\n"
            << "2. Monthly Sales Report\n"
            << "3. Yearly Sales Report\n"
            << "4. Sales Analysis\n"
            << "0. Back\n"
            << "----------------------------------------\n"
            << "Enter choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input! Please enter a number.\n";
            clearCinError();
            continue;
        }

        switch (choice) {
        case 1: dailyReport(invoices); break;
        case 2: monthlyReport(invoices); break;
        case 3: yearlyReport(invoices); break;
        case 4: salesAnalysisMenu(invoices); break;
        case 0: break;
        default: std::cout << "Invalid choice! Please select an option between 0 and 4.\n";
        }
    } while (choice != 0);
}

void dailyReport(const std::vector<Invoice>& invoices) {
    std::string dateInput;
    std::cout << "\nEnter Date (DD/MM/YYYY or YYYY-MM-DD): ";
    std::cin >> dateInput;

    ParsedDate target;
    if (!parseDate(dateInput, target)) {
        std::cout << "ERROR: Invalid or unparseable date format. Use DD/MM/YYYY or YYYY-MM-DD.\n";
        return;
    }

    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    int totalBills = 0, completedServicesCount = 0;

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        if (d.day == target.day && d.month == target.month && d.year == target.year) {
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

    std::cout << "\n========================================\n"
        << "          DAILY SALES REPORT            \n"
        << "========================================\n"
        << "Date               : " << dateInput << "\n\n"
        << "Total Bills        : " << totalBills << "\n"
        << "Total Sales        : RM" << std::fixed << std::setprecision(2) << totalSales << "\n"
        << "Payment Received   : RM" << paymentReceived << "\n"
        << "Outstanding Amount : RM" << outstandingAmount << "\n"
        << "Completed Services : " << completedServicesCount << "\n"
        << "========================================\n";
}

void monthlyReport(const std::vector<Invoice>& invoices) {
    int month = 0, year = 0;
    std::cout << "\nEnter Month (1-12): ";
    if (!(std::cin >> month) || month < 1 || month > 12) {
        std::cout << "ERROR: Invalid month input.\n";
        clearCinError();
        return;
    }
    std::cout << "Enter Year (YYYY): ";
    if (!(std::cin >> year) || year <= 0) {
        std::cout << "ERROR: Invalid year input.\n";
        clearCinError();
        return;
    }

    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    int totalBills = 0, completedServicesCount = 0;

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        if (d.month != month || d.year != year) continue;

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

    int dim = daysInMonth(month, year);
    double avgDailySales = (totalBills > 0) ? (totalSales / static_cast<double>(dim)) : 0.0;

    std::cout << "\n========================================\n"
        << "          MONTHLY SALES REPORT          \n"
        << "========================================\n"
        << "Month/Year         : " << std::setw(2) << std::setfill('0') << month << "/" << year << std::setfill(' ') << "\n\n"
        << "Total Bills        : " << totalBills << "\n"
        << "Total Sales        : RM" << std::fixed << std::setprecision(2) << totalSales << "\n"
        << "Payment Received   : RM" << paymentReceived << "\n"
        << "Outstanding Amount : RM" << outstandingAmount << "\n"
        << "Completed Services : " << completedServicesCount << "\n"
        << "Average Daily Sales: RM" << avgDailySales << "\n"
        << "========================================\n";
}

void yearlyReport(const std::vector<Invoice>& invoices) {
    int year = 0;
    std::cout << "\nEnter Year (YYYY): ";
    if (!(std::cin >> year) || year <= 0) {
        std::cout << "ERROR: Invalid year input.\n";
        clearCinError();
        return;
    }

    std::array<double, 12> monthlyTotals{};
    double totalYearlySales = 0.0;

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        if (d.year != year) continue;
        monthlyTotals[d.month - 1] += inv.totalAmount;
        totalYearlySales += inv.totalAmount;
    }

    static const char* monthNames[12] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    std::cout << "\n========================================\n"
        << "          YEARLY SALES REPORT           \n"
        << "========================================\n"
        << "Year: " << year << "\n\n";

    std::cout << std::fixed << std::setprecision(2);
    for (int i = 0; i < 12; ++i) {
        std::cout << std::left << std::setw(15) << monthNames[i] << "RM" << monthlyTotals[i] << "\n";
    }
    std::cout << "----------------------------------------\n"
        << "Total Yearly Sales: RM" << totalYearlySales << "\n"
        << "========================================\n";
}

void salesAnalysisMenu(const std::vector<Invoice>& invoices) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n"
            << "             SALES ANALYSIS             \n"
            << "========================================\n"
            << "1. Best Sales Month\n"
            << "2. Lowest Sales Month\n"
            << "3. Average Monthly Sales\n"
            << "4. Total Yearly Sales\n"
            << "5. Sales Growth\n"
            << "0. Back\n"
            << "----------------------------------------\n"
            << "Enter choice: ";

        if (!(std::cin >> choice)) {
            clearCinError();
            continue;
        }

        if (choice < 0 || choice > 5) {
            std::cout << "Invalid choice!\n";
            continue;
        }
        if (choice == 0) break;

        if (invoices.empty()) {
            std::cout << "\nNo invoice data available for analysis.\n";
            continue;
        }

        std::vector<int> years;
        std::vector<std::array<double, 12>> perYearTotals;

        auto findOrAddYear = [&](int y) -> std::array<double, 12>&{
            for (size_t i = 0; i < years.size(); ++i) {
                if (years[i] == y) return perYearTotals[i];
            }
            years.push_back(y);
            perYearTotals.push_back(std::array<double, 12>{});
            perYearTotals.back().fill(0.0);
            return perYearTotals.back();
            };

        double totalSales = 0.0;
        for (const auto& inv : invoices) {
            ParsedDate d;
            if (!parseDate(inv.date, d)) continue;
            findOrAddYear(d.year)[d.month - 1] += inv.totalAmount;
            totalSales += inv.totalAmount;
        }

        static const char* monthNames[12] = {
            "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        };

        struct MonthEntry { int year; int month; double total; };
        std::vector<MonthEntry> entries;
        for (size_t i = 0; i < years.size(); ++i) {
            for (int m = 0; m < 12; ++m) {
                if (perYearTotals[i][m] > 0.0) {
                    entries.push_back({ years[i], m, perYearTotals[i][m] });
                }
            }
        }

        switch (choice) {
        case 1: {
            if (entries.empty()) { std::cout << "\nNo sales data available.\n"; break; }
            auto best = std::max_element(entries.begin(), entries.end(),
                [](const MonthEntry& a, const MonthEntry& b) { return a.total < b.total; });
            std::cout << "\nBest Sales Month: " << monthNames[best->month] << " " << best->year
                << " (RM" << std::fixed << std::setprecision(2) << best->total << ")\n";
            break;
        }
        case 2: {
            if (entries.empty()) { std::cout << "\nNo sales data available.\n"; break; }
            auto worst = std::min_element(entries.begin(), entries.end(),
                [](const MonthEntry& a, const MonthEntry& b) { return a.total < b.total; });
            std::cout << "\nLowest Sales Month: " << monthNames[worst->month] << " " << worst->year
                << " (RM" << std::fixed << std::setprecision(2) << worst->total << ")\n";
            break;
        }
        case 3: {
            int activeMonths = static_cast<int>(entries.size());
            double avg = activeMonths > 0 ? (totalSales / activeMonths) : 0.0;
            std::cout << "\nAverage Monthly Sales: RM" << std::fixed << std::setprecision(2) << avg << "\n";
            break;
        }
        case 4:
            std::cout << "\nTotal Sales (All Recorded Invoices): RM" << std::fixed << std::setprecision(2) << totalSales << "\n";
            break;
        case 5: {
            if (years.size() < 2) {
                std::cout << "\nSales Growth: Not enough historical data (need at least 2 distinct years).\n";
                break;
            }
            std::vector<int> sortedYears = years;
            std::sort(sortedYears.begin(), sortedYears.end());
            int latestYear = sortedYears.back();
            int prevYear = sortedYears[sortedYears.size() - 2];

            double latestTotal = 0.0, prevTotal = 0.0;
            for (size_t i = 0; i < years.size(); ++i) {
                if (years[i] == latestYear) for (double v : perYearTotals[i]) latestTotal += v;
                if (years[i] == prevYear) for (double v : perYearTotals[i]) prevTotal += v;
            }

            if (prevTotal <= 0.0) {
                std::cout << "\nSales Growth: Cannot compute (previous year " << prevYear << " has no recorded sales).\n";
            }
            else {
                double growthPct = ((latestTotal - prevTotal) / prevTotal) * 100.0;
                std::cout << "\nSales Growth (" << prevYear << " -> " << latestYear << "): "
                    << std::fixed << std::setprecision(1) << std::showpos << growthPct << std::noshowpos << "%\n";
            }
            break;
        }
        }
    } while (choice != 0);
}

void customerStatistics(const std::vector<Customer>& customers) {
    int statsMatrix[4][2] = { {0} }; // 0: Regular, 1: Silver, 2: Gold, 3: VIP

    for (const auto& c : customers) {
        int rowIdx = 0;
        if (c.membershipTier == "Silver") rowIdx = 1;
        else if (c.membershipTier == "Gold") rowIdx = 2;
        else if (c.membershipTier == "VIP") rowIdx = 3;

        statsMatrix[rowIdx][0]++;
        statsMatrix[rowIdx][1] += static_cast<int>(c.vehicles.size());
    }

    std::cout << "\n--- CUSTOMER TIER & VEHICLE STATISTICS MATRIX ---\n"
        << std::left << std::setw(15) << "Tier"
        << std::setw(18) << "Total Customers"
        << "Total Cars\n"
        << "---------------------------------------------------------\n"
        << std::left << std::setw(15) << "Regular" << std::setw(18) << statsMatrix[0][0] << statsMatrix[0][1] << "\n"
        << std::left << std::setw(15) << "Silver" << std::setw(18) << statsMatrix[1][0] << statsMatrix[1][1] << "\n"
        << std::left << std::setw(15) << "Gold" << std::setw(18) << statsMatrix[2][0] << statsMatrix[2][1] << "\n"
        << std::left << std::setw(15) << "VIP" << std::setw(18) << statsMatrix[3][0] << statsMatrix[3][1] << "\n";
}

void exportReports(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    const std::string filename = "exported_report.txt";
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << "ERROR: Could not open " << filename << " for writing!\n";
        return;
    }

    outFile << std::fixed << std::setprecision(2)
        << "=========================================================\n"
        << "        CAR WORKSHOP - CONSOLIDATED REPORT EXPORT        \n"
        << "=========================================================\n\n";

    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    int totalBills = 0, paidBills = 0;
    for (const auto& inv : invoices) {
        totalBills++;
        totalSales += inv.totalAmount;
        if (inv.isPaid) {
            paymentReceived += inv.totalAmount;
            paidBills++;
        }
        else {
            outstandingAmount += inv.totalAmount;
        }
    }

    outFile << "-- OVERALL TOTALS --\n"
        << "Total Bills        : " << totalBills << "\n"
        << "Paid Bills         : " << paidBills << "\n"
        << "Total Sales        : RM" << totalSales << "\n"
        << "Payment Received   : RM" << paymentReceived << "\n"
        << "Outstanding Amount : RM" << outstandingAmount << "\n\n";

    std::vector<int> years;
    std::vector<std::array<double, 12>> perYearTotals;
    auto findOrAddYear = [&](int y) -> std::array<double, 12>&{
        for (size_t i = 0; i < years.size(); ++i) {
            if (years[i] == y) return perYearTotals[i];
        }
        years.push_back(y);
        perYearTotals.push_back(std::array<double, 12>{});
        perYearTotals.back().fill(0.0);
        return perYearTotals.back();
        };

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        findOrAddYear(d.year)[d.month - 1] += inv.totalAmount;
    }

    static const char* monthNames[12] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    std::vector<int> sortedYears = years;
    std::sort(sortedYears.begin(), sortedYears.end());

    for (int y : sortedYears) {
        double yearTotal = 0.0;
        std::array<double, 12>* totals = nullptr;
        for (size_t i = 0; i < years.size(); ++i) {
            if (years[i] == y) totals = &perYearTotals[i];
        }
        outFile << "-- YEAR " << y << " --\n";
        for (int m = 0; m < 12; ++m) {
            outFile << std::left << std::setw(15) << monthNames[m] << "RM" << (*totals)[m] << "\n";
            yearTotal += (*totals)[m];
        }
        outFile << "Year Total         : RM" << yearTotal << "\n\n";
    }

    if (sortedYears.empty()) {
        outFile << "(No dated sales records found.)\n\n";
    }

    int statsMatrix[4][2] = { {0} };
    for (const auto& c : customers) {
        int rowIdx = 0;
        if (c.membershipTier == "Silver") rowIdx = 1;
        else if (c.membershipTier == "Gold") rowIdx = 2;
        else if (c.membershipTier == "VIP") rowIdx = 3;
        statsMatrix[rowIdx][0]++;
        statsMatrix[rowIdx][1] += static_cast<int>(c.vehicles.size());
    }

    outFile << "-- CUSTOMER TIER & VEHICLE STATISTICS --\n"
        << std::left << std::setw(15) << "Tier" << std::setw(18) << "Total Customers" << "Total Cars\n"
        << std::left << std::setw(15) << "Regular" << std::setw(18) << statsMatrix[0][0] << statsMatrix[0][1] << "\n"
        << std::left << std::setw(15) << "Silver" << std::setw(18) << statsMatrix[1][0] << statsMatrix[1][1] << "\n"
        << std::left << std::setw(15) << "Gold" << std::setw(18) << statsMatrix[2][0] << statsMatrix[2][1] << "\n"
        << std::left << std::setw(15) << "VIP" << std::setw(18) << statsMatrix[3][0] << statsMatrix[3][1] << "\n";

    outFile.close();
    std::cout << "\nReport successfully exported to " << filename << "\n";
}