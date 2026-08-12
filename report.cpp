#include "report.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <array>
#include <limits>
#include <fstream>

// ---------------------------------------------------------------------------
// Date parsing helper
// ---------------------------------------------------------------------------
// Supports two accepted input formats:
//   DD/MM/YYYY   (e.g. 05/03/2026)
//   YYYY-MM-DD   (e.g. 2026-03-05)
// Returns false if the string could not be parsed in either format.
struct ParsedDate {
    int day = 0;
    int month = 0;
    int year = 0;
};

static bool parseDate(const std::string& raw, ParsedDate& out) {
    // Try DD/MM/YYYY
    {
        std::stringstream ss(raw);
        int d, m, y;
        char sep1, sep2;
        if ((ss >> d >> sep1 >> m >> sep2 >> y) && sep1 == '/' && sep2 == '/') {
            if (d >= 1 && d <= 31 && m >= 1 && m <= 12 && y > 0) {
                out = { d, m, y };
                return true;
            }
        }
    }
    // Try YYYY-MM-DD
    {
        std::stringstream ss(raw);
        int y, m, d;
        char sep1, sep2;
        if ((ss >> y >> sep1 >> m >> sep2 >> d) && sep1 == '-' && sep2 == '-') {
            if (d >= 1 && d <= 31 && m >= 1 && m <= 12 && y > 0) {
                out = { d, m, y };
                return true;
            }
        }
    }
    return false;
}

// Helper: returns days in a given month/year, accounting for leap years
static int daysInMonth(int month, int year) {
    static const int base[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month < 1 || month > 12) return 30; // fallback
    if (month == 2) {
        bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        return leap ? 29 : 28;
    }
    return base[month - 1];
}

void reportMenu(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "         REPORTS & ANALYTICS            \n";
        std::cout << "========================================\n";
        std::cout << "1. All Reports Summary (daily, monthly, yearly)\n";
        std::cout << "2. Customer Statistics\n";
        std::cout << "3. Export Reports\n";
        std::cout << "0. Return to Main Menu\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Enter choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input! Please enter a number.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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

// Sub-menu for "1. All Reports Summary" - groups the daily/monthly/yearly
// breakdowns plus the sales-analysis view (best/worst month, growth) that
// used to live at the top level, since the menu diagram folds all of these
// under one summary entry.
void allReportsSummaryMenu(const std::vector<Invoice>& invoices) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "          ALL REPORTS SUMMARY            \n";
        std::cout << "========================================\n";
        std::cout << "1. Daily Sales Report\n";
        std::cout << "2. Monthly Sales Report\n";
        std::cout << "3. Yearly Sales Report\n";
        std::cout << "4. Sales Analysis\n";
        std::cout << "0. Back\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Enter choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input! Please enter a number.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
        std::cout << "ERROR: Could not parse date '" << dateInput << "'. Use DD/MM/YYYY or YYYY-MM-DD.\n";
        return;
    }

    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    int totalBills = 0;
    int completedServicesCount = 0;

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue; // skip unparsable records rather than crash
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
    if (!(std::cin >> month) || month < 1 || month > 12) {
        std::cout << "ERROR: Invalid month.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    std::cout << "Enter Year (YYYY): ";
    if (!(std::cin >> year) || year <= 0) {
        std::cout << "ERROR: Invalid year.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    int totalBills = 0;
    int completedServicesCount = 0;

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        if (d.month != month || d.year != year) continue; // <-- actual filter, was missing before

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
    if (!(std::cin >> year) || year <= 0) {
        std::cout << "ERROR: Invalid year.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    std::array<double, 12> monthlyTotals{};
    monthlyTotals.fill(0.0);
    double totalYearlySales = 0.0;

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        if (d.year != year) continue; // <-- actual filter, was missing before
        monthlyTotals[d.month - 1] += inv.totalAmount;
        totalYearlySales += inv.totalAmount;
    }

    static const char* monthNames[12] = {
        "January","February","March","April","May","June",
        "July","August","September","October","November","December"
    };

    std::cout << "\n========================================\n";
    std::cout << "          YEARLY SALES REPORT           \n";
    std::cout << "========================================\n";
    std::cout << "Year: " << year << "\n\n";
    std::cout << std::fixed << std::setprecision(2);
    for (int i = 0; i < 12; ++i) {
        std::cout << std::left << std::setw(15) << monthNames[i] << "RM" << monthlyTotals[i] << "\n";
    }
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
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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

        // Build a year -> 12-month totals map by scanning all invoices once.
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
            "January","February","March","April","May","June",
            "July","August","September","October","November","December"
        };

        // Flatten all (year, month, total) combos that actually have sales.
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
            // Growth = most recent year's total vs the year before it, if both exist.
            if (years.size() < 2) {
                std::cout << "\nSales Growth: Not enough historical data (need at least 2 years).\n";
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

// Writes a single consolidated report - overall totals, a full yearly/monthly
// sales breakdown across every year present in the data, and the customer
// tier & vehicle matrix - out to a text file so it can be shared/archived
// outside the console.
void exportReports(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    const std::string filename = "exported_report.txt";
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << "ERROR: Could not open " << filename << " for writing!\n";
        return;
    }

    outFile << std::fixed << std::setprecision(2);
    outFile << "=========================================================\n";
    outFile << "         CAR WORKSHOP - CONSOLIDATED REPORT EXPORT       \n";
    outFile << "=========================================================\n\n";

    // ---- Overall totals ----
    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    int totalBills = 0, paidBills = 0;
    for (const auto& inv : invoices) {
        totalBills++;
        totalSales += inv.totalAmount;
        if (inv.isPaid) { paymentReceived += inv.totalAmount; paidBills++; }
        else { outstandingAmount += inv.totalAmount; }
    }
    outFile << "-- OVERALL TOTALS --\n";
    outFile << "Total Bills        : " << totalBills << "\n";
    outFile << "Paid Bills         : " << paidBills << "\n";
    outFile << "Total Sales        : RM" << totalSales << "\n";
    outFile << "Payment Received   : RM" << paymentReceived << "\n";
    outFile << "Outstanding Amount : RM" << outstandingAmount << "\n\n";

    // ---- Per-year monthly breakdown (mirrors yearlyReport, for every year seen) ----
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
        "January","February","March","April","May","June",
        "July","August","September","October","November","December"
    };
    std::vector<int> sortedYears = years;
    std::sort(sortedYears.begin(), sortedYears.end());
    for (int y : sortedYears) {
        double yearTotal = 0.0;
        std::array<double, 12>* totals = nullptr;
        for (size_t i = 0; i < years.size(); ++i) if (years[i] == y) totals = &perYearTotals[i];
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

    // ---- Customer tier & vehicle statistics ----
    int statsMatrix[4][2] = { {0} };
    for (const auto& c : customers) {
        int rowIdx = 0;
        if (c.membershipTier == "Silver") rowIdx = 1;
        else if (c.membershipTier == "Gold") rowIdx = 2;
        else if (c.membershipTier == "VIP") rowIdx = 3;
        statsMatrix[rowIdx][0]++;
        statsMatrix[rowIdx][1] += static_cast<int>(c.vehicles.size());
    }
    outFile << "-- CUSTOMER TIER & VEHICLE STATISTICS --\n";
    outFile << std::left << std::setw(15) << "Tier" << std::setw(18) << "Total Customers" << "Total Cars\n";
    outFile << std::left << std::setw(15) << "Regular" << std::setw(18) << statsMatrix[0][0] << statsMatrix[0][1] << "\n";
    outFile << std::left << std::setw(15) << "Silver" << std::setw(18) << statsMatrix[1][0] << statsMatrix[1][1] << "\n";
    outFile << std::left << std::setw(15) << "Gold" << std::setw(18) << statsMatrix[2][0] << statsMatrix[2][1] << "\n";
    outFile << std::left << std::setw(15) << "VIP" << std::setw(18) << statsMatrix[3][0] << statsMatrix[3][1] << "\n";

    outFile.close();
    std::cout << "\nReport successfully exported to " << filename << "\n";
}