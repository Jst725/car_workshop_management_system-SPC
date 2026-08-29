#include "report.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <array>
#include <limits>
#include <fstream>
#include <ctime>
#include <cstdlib>

// ANSI Light Color Definitions (No Blue, Green, or Purple)
namespace Color {
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string CYAN = "\033[36m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_WHITE = "\033[97m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string LIGHT_GRAY = "\033[37m";
}

// Helper: Cross-platform screen clear function
static void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Helper: Cross-platform pause function
static void pauseScreen() {
    std::cout << Color::BRIGHT_YELLOW << "\nPress Enter to continue..." << Color::RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

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
        clearScreen();
        std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
            << Color::BRIGHT_YELLOW << Color::BOLD << "          REPORTS & ANALYTICS           \n" << Color::RESET
            << Color::CYAN << "========================================\n" << Color::RESET
            << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "All Reports Summary (Daily, Monthly, Yearly)\n"
            << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Customer Statistics\n"
            << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Export Reports\n"
            << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Return to Main Menu\n"
            << Color::CYAN << "----------------------------------------\n" << Color::RESET
            << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;

        if (!(std::cin >> choice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
            clearCinError();
            pauseScreen();
            continue;
        }

        switch (choice) {
        case 1: allReportsSummaryMenu(invoices); break;
        case 2: clearScreen(); customerStatistics(customers); break;
        case 3: clearScreen(); exportReports(invoices, customers); break;
        case 0: std::cout << Color::BRIGHT_CYAN << "Returning to Main Menu...\n" << Color::RESET; break;
        default: std::cout << Color::BRIGHT_RED << "Invalid choice! Please select an option between 0 and 3.\n" << Color::RESET; pauseScreen();
        }
    } while (choice != 0);
}

void allReportsSummaryMenu(const std::vector<Invoice>& invoices) {
    int choice = -1;
    do {
        clearScreen();
        std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
            << Color::BRIGHT_YELLOW << Color::BOLD << "          ALL REPORTS SUMMARY           \n" << Color::RESET
            << Color::CYAN << "========================================\n" << Color::RESET
            << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Daily Sales Report\n"
            << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Monthly Sales Report\n"
            << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Yearly Sales Report\n"
            << Color::BRIGHT_CYAN << "4. " << Color::BRIGHT_WHITE << "Sales Analysis\n"
            << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Back\n"
            << Color::CYAN << "----------------------------------------\n" << Color::RESET
            << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;

        if (!(std::cin >> choice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
            clearCinError();
            pauseScreen();
            continue;
        }

        switch (choice) {
        case 1: clearScreen(); dailyReport(invoices); break;
        case 2: clearScreen(); monthlyReport(invoices); break;
        case 3: clearScreen(); yearlyReport(invoices); break;
        case 4: salesAnalysisMenu(invoices); break;
        case 0: break;
        default: std::cout << Color::BRIGHT_RED << "Invalid choice! Please select an option between 0 and 4.\n" << Color::RESET; pauseScreen();
        }
    } while (choice != 0);
}

void dailyReport(const std::vector<Invoice>& invoices) {
    std::string dateInput;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Date (DD/MM/YYYY or YYYY-MM-DD): " << Color::RESET;
    std::cin >> dateInput;

    ParsedDate target;
    if (!parseDate(dateInput, target)) {
        std::cout << Color::BRIGHT_RED << "ERROR: Invalid or unparseable date format. Use DD/MM/YYYY or YYYY-MM-DD.\n" << Color::RESET;
        pauseScreen();
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

    std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
        << Color::BRIGHT_YELLOW << Color::BOLD << "          DAILY SALES REPORT            \n" << Color::RESET
        << Color::CYAN << "========================================\n" << Color::RESET
        << Color::BRIGHT_CYAN << "Date               : " << Color::BRIGHT_WHITE << dateInput << "\n\n"
        << Color::BRIGHT_CYAN << "Total Bills        : " << Color::BRIGHT_WHITE << totalBills << "\n"
        << Color::BRIGHT_CYAN << "Total Sales        : RM" << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << totalSales << "\n"
        << Color::BRIGHT_CYAN << "Payment Received   : RM" << Color::BRIGHT_WHITE << paymentReceived << "\n"
        << Color::BRIGHT_CYAN << "Outstanding Amount : RM" << Color::BRIGHT_WHITE << outstandingAmount << "\n"
        << Color::BRIGHT_CYAN << "Completed Services : " << Color::BRIGHT_WHITE << completedServicesCount << "\n"
        << Color::CYAN << "========================================\n" << Color::RESET;

    pauseScreen();
}

void monthlyReport(const std::vector<Invoice>& invoices) {
    int month = 0, year = 0;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Month (1-12): " << Color::RESET;
    if (!(std::cin >> month) || month < 1 || month > 12) {
        std::cout << Color::BRIGHT_RED << "ERROR: Invalid month input.\n" << Color::RESET;
        clearCinError();
        pauseScreen();
        return;
    }
    std::cout << Color::BRIGHT_YELLOW << "Enter Year (YYYY): " << Color::RESET;
    if (!(std::cin >> year) || year <= 0) {
        std::cout << Color::BRIGHT_RED << "ERROR: Invalid year input.\n" << Color::RESET;
        clearCinError();
        pauseScreen();
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

    std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
        << Color::BRIGHT_YELLOW << Color::BOLD << "          MONTHLY SALES REPORT          \n" << Color::RESET
        << Color::CYAN << "========================================\n" << Color::RESET
        << Color::BRIGHT_CYAN << "Month/Year         : " << Color::BRIGHT_WHITE << std::setw(2) << std::setfill('0') << month << std::setfill(' ') << "/" << year << "\n\n"
        << Color::BRIGHT_CYAN << "Total Bills        : " << Color::BRIGHT_WHITE << totalBills << "\n"
        << Color::BRIGHT_CYAN << "Total Sales        : RM" << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << totalSales << "\n"
        << Color::BRIGHT_CYAN << "Payment Received   : RM" << Color::BRIGHT_WHITE << paymentReceived << "\n"
        << Color::BRIGHT_CYAN << "Outstanding Amount : RM" << Color::BRIGHT_WHITE << outstandingAmount << "\n"
        << Color::BRIGHT_CYAN << "Completed Services : " << Color::BRIGHT_WHITE << completedServicesCount << "\n"
        << Color::BRIGHT_CYAN << "Average Daily Sales: RM" << Color::BRIGHT_WHITE << avgDailySales << "\n"
        << Color::CYAN << "========================================\n" << Color::RESET;

    pauseScreen();
}

void yearlyReport(const std::vector<Invoice>& invoices) {
    int year = 0;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Year (YYYY): " << Color::RESET;
    if (!(std::cin >> year) || year <= 0) {
        std::cout << Color::BRIGHT_RED << "ERROR: Invalid year input.\n" << Color::RESET;
        clearCinError();
        pauseScreen();
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

    std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
        << Color::BRIGHT_YELLOW << Color::BOLD << "          YEARLY SALES REPORT           \n" << Color::RESET
        << Color::CYAN << "========================================\n" << Color::RESET
        << Color::BRIGHT_CYAN << "Year: " << Color::BRIGHT_WHITE << year << "\n\n" << Color::RESET;

    std::cout << std::fixed << std::setprecision(2);
    for (int i = 0; i < 12; ++i) {
        std::cout << Color::BRIGHT_CYAN << std::left << std::setw(15) << monthNames[i]
            << Color::BRIGHT_WHITE << "RM" << monthlyTotals[i] << "\n" << Color::RESET;
    }
    std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET
        << Color::BRIGHT_YELLOW << "Total Yearly Sales: RM" << Color::BRIGHT_WHITE << totalYearlySales << "\n"
        << Color::CYAN << "========================================\n" << Color::RESET;

    pauseScreen();
}

void salesAnalysisMenu(const std::vector<Invoice>& invoices) {
    int choice = -1;
    do {
        clearScreen();
        std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
            << Color::BRIGHT_YELLOW << Color::BOLD << "             SALES ANALYSIS             \n" << Color::RESET
            << Color::CYAN << "========================================\n" << Color::RESET
            << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Best Sales Month\n"
            << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Lowest Sales Month\n"
            << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Average Monthly Sales\n"
            << Color::BRIGHT_CYAN << "4. " << Color::BRIGHT_WHITE << "Total Yearly Sales\n"
            << Color::BRIGHT_CYAN << "5. " << Color::BRIGHT_WHITE << "Sales Growth\n"
            << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Back\n"
            << Color::CYAN << "----------------------------------------\n" << Color::RESET
            << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;

        if (!(std::cin >> choice)) {
            clearCinError();
            pauseScreen();
            continue;
        }

        if (choice < 0 || choice > 5) {
            std::cout << Color::BRIGHT_RED << "Invalid choice!\n" << Color::RESET;
            pauseScreen();
            continue;
        }
        if (choice == 0) break;

        if (invoices.empty()) {
            std::cout << Color::BRIGHT_YELLOW << "\nNo invoice data available for analysis.\n" << Color::RESET;
            pauseScreen();
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
            if (entries.empty()) { std::cout << Color::BRIGHT_YELLOW << "\nNo sales data available.\n" << Color::RESET; break; }
            auto best = std::max_element(entries.begin(), entries.end(),
                [](const MonthEntry& a, const MonthEntry& b) { return a.total < b.total; });
            std::cout << Color::BRIGHT_CYAN << "\nBest Sales Month: " << Color::BRIGHT_WHITE << monthNames[best->month] << " " << best->year
                << " (RM" << std::fixed << std::setprecision(2) << best->total << ")\n" << Color::RESET;
            break;
        }
        case 2: {
            if (entries.empty()) { std::cout << Color::BRIGHT_YELLOW << "\nNo sales data available.\n" << Color::RESET; break; }
            auto worst = std::min_element(entries.begin(), entries.end(),
                [](const MonthEntry& a, const MonthEntry& b) { return a.total < b.total; });
            std::cout << Color::BRIGHT_CYAN << "\nLowest Sales Month: " << Color::BRIGHT_WHITE << monthNames[worst->month] << " " << worst->year
                << " (RM" << std::fixed << std::setprecision(2) << worst->total << ")\n" << Color::RESET;
            break;
        }
        case 3: {
            int activeMonths = static_cast<int>(entries.size());
            double avg = activeMonths > 0 ? (totalSales / activeMonths) : 0.0;
            std::cout << Color::BRIGHT_CYAN << "\nAverage Monthly Sales: " << Color::BRIGHT_WHITE << "RM" << std::fixed << std::setprecision(2) << avg << "\n" << Color::RESET;
            break;
        }
        case 4:
            std::cout << Color::BRIGHT_CYAN << "\nTotal Sales (All Recorded Invoices): " << Color::BRIGHT_WHITE << "RM" << std::fixed << std::setprecision(2) << totalSales << "\n" << Color::RESET;
            break;
        case 5: {
            if (years.size() < 2) {
                std::cout << Color::BRIGHT_YELLOW << "\nSales Growth: Not enough historical data (need at least 2 distinct years).\n" << Color::RESET;
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
                std::cout << Color::BRIGHT_YELLOW << "\nSales Growth: Cannot compute (previous year " << prevYear << " has no recorded sales).\n" << Color::RESET;
            }
            else {
                double growthPct = ((latestTotal - prevTotal) / prevTotal) * 100.0;
                std::cout << Color::BRIGHT_CYAN << "\nSales Growth (" << prevYear << " -> " << latestYear << "): "
                    << Color::BRIGHT_WHITE << std::fixed << std::setprecision(1) << std::showpos << growthPct << std::noshowpos << "%\n" << Color::RESET;
            }
            break;
        }
        }
        pauseScreen();
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

    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "\n--- CUSTOMER TIER & VEHICLE STATISTICS MATRIX ---\n" << Color::RESET
        << Color::BRIGHT_CYAN << std::left << std::setw(15) << "Tier"
        << std::setw(18) << "Total Customers"
        << "Total Cars\n"
        << Color::CYAN << "---------------------------------------------------------\n" << Color::RESET
        << Color::BRIGHT_WHITE << std::left << std::setw(15) << "Regular" << std::setw(18) << statsMatrix[0][0] << statsMatrix[0][1] << "\n"
        << std::left << std::setw(15) << "Silver" << std::setw(18) << statsMatrix[1][0] << statsMatrix[1][1] << "\n"
        << std::left << std::setw(15) << "Gold" << std::setw(18) << statsMatrix[2][0] << statsMatrix[2][1] << "\n"
        << std::left << std::setw(15) << "VIP" << std::setw(18) << statsMatrix[3][0] << statsMatrix[3][1] << "\n" << Color::RESET;

    pauseScreen();
}

void exportReports(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    const std::string filename = "exported_report.txt";
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << Color::BRIGHT_RED << "ERROR: Could not open " << filename << " for writing!\n" << Color::RESET;
        pauseScreen();
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
    std::cout << Color::BRIGHT_CYAN << "\nReport successfully exported to " << Color::BRIGHT_WHITE << filename << "\n" << Color::RESET;

    pauseScreen();
}