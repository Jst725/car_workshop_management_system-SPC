#include "report.h"
#include "structures.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <array>
#include <limits>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <string>

// ANSI Light Color Definitions
namespace Color {
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string CYAN = "\033[36m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_WHITE = "\033[97m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string LIGHT_GRAY = "\033[37m";
    const std::string BRIGHT_GREEN = "\033[92m";
}

// Helper functions
static void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void pauseScreen() {
    std::cout << Color::BRIGHT_YELLOW << "\nPress Enter to continue..." << Color::RESET;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

struct ParsedDate {
    int day = 0;
    int month = 0;
    int year = 0;
};

static bool parseDate(const std::string& raw, ParsedDate& out) {
    if (raw.size() < 8) return false;

    int d = 0, m = 0, y = 0;
    char sep1 = '\0', sep2 = '\0';

    // Try DD/MM/YYYY
    std::stringstream ss(raw);
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
    std::stringstream ss2(raw);
    if ((ss2 >> y >> sep1 >> m >> sep2 >> d) && sep1 == '-' && sep2 == '-') {
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

static void clearCinError() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static int daysInMonth(int month, int year) {
    static const int base[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2) {
        bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        return leap ? 29 : 28;
    }
    return (month >= 1 && month <= 12) ? base[month - 1] : 30;
}

static std::string getCustomerName(const std::vector<Customer>& customers, const std::string& phone) {
    for (const auto& c : customers) {
        if (c.phone == phone) {
            return c.name;
        }
    }
    return "Unknown Customer";
}

static std::string getCurrentDateString() {
    std::time_t t = std::time(nullptr);
    std::tm now{};
#ifdef _WIN32
    localtime_s(&now, &t);
#else
    now = *std::localtime(&t);
#endif
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &now);
    return std::string(buffer);
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
            << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Sales Analysis\n"
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
        case 1: allReportsSummaryMenu(invoices, customers); break;
        case 2: salesAnalysisMenu(invoices); break;
        case 3: exportReports(invoices, customers); break;
        case 0: std::cout << Color::BRIGHT_CYAN << "Returning to Main Menu...\n" << Color::RESET; break;
        default: std::cout << Color::BRIGHT_RED << "Invalid choice! Please select an option between 0 and 3.\n" << Color::RESET; pauseScreen();
        }
    } while (choice != 0);
}

void allReportsSummaryMenu(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    int choice = -1;
    do {
        clearScreen();
        std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
            << Color::BRIGHT_YELLOW << Color::BOLD << "          ALL REPORTS SUMMARY           \n" << Color::RESET
            << Color::CYAN << "========================================\n" << Color::RESET
            << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Daily Sales Report\n"
            << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Monthly Sales Report\n"
            << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Yearly Sales Report\n"
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
        case 1: dailyReport(invoices, customers); break;
        case 2: monthlyReport(invoices, customers); break;
        case 3: yearlyReport(invoices); break;
        case 0: break;
        default: std::cout << Color::BRIGHT_RED << "Invalid choice! Please select an option between 0 and 3.\n" << Color::RESET; pauseScreen();
        }
    } while (choice != 0);
}

void dailyReport(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    if (invoices.empty()) {
        std::cout << Color::BRIGHT_YELLOW << "\n[INFO] No invoice data available.\n" << Color::RESET;
        pauseScreen();
        return;
    }

    std::string dateInput;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Date (DD/MM/YYYY or YYYY-MM-DD): " << Color::RESET;
    std::cin >> dateInput;

    if (dateInput.empty()) {
        std::cout << Color::BRIGHT_RED << "ERROR: No date entered.\n" << Color::RESET;
        pauseScreen();
        return;
    }

    ParsedDate target;
    if (!parseDate(dateInput, target)) {
        std::cout << Color::BRIGHT_RED << "ERROR: Invalid date format. Use DD/MM/YYYY or YYYY-MM-DD.\n" << Color::RESET;
        pauseScreen();
        return;
    }

    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    double totalTax = 0.0;
    int totalBills = 0, completedServicesCount = 0, unpaidBills = 0;
    std::vector<const Invoice*> dailyInvoices;

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        if (d.day == target.day && d.month == target.month && d.year == target.year) {
            totalBills++;
            dailyInvoices.push_back(&inv);
            totalSales += inv.totalAmount;
            totalTax += inv.taxAmount;
            if (inv.isPaid) {
                paymentReceived += inv.totalAmount;
                completedServicesCount++;
            }
            else {
                outstandingAmount += inv.totalAmount;
                unpaidBills++;
            }
        }
    }

    std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
        << Color::BRIGHT_YELLOW << Color::BOLD << "          DAILY SALES REPORT            \n" << Color::RESET
        << Color::CYAN << "========================================\n" << Color::RESET
        << Color::BRIGHT_CYAN << "Date               : " << Color::BRIGHT_WHITE << dateInput << "\n\n";

    if (totalBills == 0) {
        std::cout << Color::BRIGHT_YELLOW << "[INFO] No invoices found for this date.\n" << Color::RESET;
    }
    else {
        std::cout << Color::BRIGHT_CYAN << "Total Bills        : " << Color::BRIGHT_WHITE << totalBills << "\n"
            << Color::BRIGHT_CYAN << "Total Sales        : RM" << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << totalSales << "\n"
            << Color::BRIGHT_CYAN << "Payment Received   : RM" << Color::BRIGHT_WHITE << paymentReceived << "\n"
            << Color::BRIGHT_CYAN << "Outstanding Amount : RM" << Color::BRIGHT_WHITE << outstandingAmount << "\n"
            << Color::BRIGHT_CYAN << "Total Tax (SST 6%) : RM" << Color::BRIGHT_WHITE << totalTax << "\n"
            << Color::BRIGHT_CYAN << "Completed Services : " << Color::BRIGHT_WHITE << completedServicesCount << "\n"
            << Color::BRIGHT_CYAN << "Unpaid Bills       : " << Color::BRIGHT_WHITE << unpaidBills << "\n\n";

        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "--- ITEMIZED INVOICE LIST ---\n" << Color::RESET;
        std::cout << Color::BRIGHT_CYAN << std::left << std::setw(12) << "Bill ID"
            << std::setw(20) << "Customer"
            << std::setw(12) << "Plate"
            << std::setw(14) << "Amount (RM)"
            << "Status\n" << Color::RESET;
        std::cout << Color::CYAN << "---------------------------------------------------------\n" << Color::RESET;

        for (const auto* inv : dailyInvoices) {
            if (inv == nullptr) continue;
            std::string custName = getCustomerName(customers, inv->customerId);
            std::cout << Color::BRIGHT_WHITE << std::left << std::setw(12) << inv->invoiceId
                << std::setw(20) << custName
                << std::setw(12) << inv->licensePlate
                << std::setw(14) << std::fixed << std::setprecision(2) << inv->totalAmount
                << (inv->isPaid ? (Color::BRIGHT_CYAN + "PAID" + Color::RESET) : (Color::BRIGHT_RED + "UNPAID" + Color::RESET)) << "\n";
        }
        std::cout << Color::CYAN << "========================================\n" << Color::RESET;
    }
    pauseScreen();
}

void monthlyReport(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    if (invoices.empty()) {
        std::cout << Color::BRIGHT_YELLOW << "\n[INFO] No invoice data available.\n" << Color::RESET;
        pauseScreen();
        return;
    }

    int month = 0, year = 0;

    std::cout << Color::BRIGHT_YELLOW << "\nEnter Month (1-12): " << Color::RESET;
    if (!(std::cin >> month)) {
        std::cout << Color::BRIGHT_RED << "ERROR: Invalid input. Please enter a number.\n" << Color::RESET;
        clearCinError();
        pauseScreen();
        return;
    }

    if (month < 1 || month > 12) {
        std::cout << Color::BRIGHT_RED << "ERROR: Month must be between 1 and 12. You entered: " << month << "\n" << Color::RESET;
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
    std::cin.ignore();

    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    double totalTax = 0.0;
    int totalBills = 0, completedServicesCount = 0, unpaidBills = 0;
    int cashPayments = 0, cardPayments = 0, ewalletPayments = 0;
    double cashAmount = 0.0, cardAmount = 0.0, ewalletAmount = 0.0;

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        if (d.month != month || d.year != year) continue;

        totalBills++;
        totalSales += inv.totalAmount;
        totalTax += inv.taxAmount;

        if (inv.isPaid) {
            paymentReceived += inv.totalAmount;
            completedServicesCount++;

            if (inv.paymentType == "Cash") {
                cashPayments++;
                cashAmount += inv.totalAmount;
            }
            else if (inv.paymentType == "Card") {
                cardPayments++;
                cardAmount += inv.totalAmount;
            }
            else if (inv.paymentType == "E-Wallet") {
                ewalletPayments++;
                ewalletAmount += inv.totalAmount;
            }
        }
        else {
            outstandingAmount += inv.totalAmount;
            unpaidBills++;
        }
    }

    int dim = daysInMonth(month, year);
    double avgDailySales = (dim > 0) ? (totalSales / static_cast<double>(dim)) : 0.0;

    std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
        << Color::BRIGHT_YELLOW << Color::BOLD << "          MONTHLY SALES REPORT          \n" << Color::RESET
        << Color::CYAN << "========================================\n" << Color::RESET
        << Color::BRIGHT_CYAN << "Month/Year         : " << Color::BRIGHT_WHITE
        << std::setw(2) << std::setfill('0') << month << "/" << year << std::setfill(' ') << "\n\n"
        << Color::BRIGHT_CYAN << "Total Bills        : " << Color::BRIGHT_WHITE << totalBills << "\n"
        << Color::BRIGHT_CYAN << "Total Sales        : RM" << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << totalSales << "\n"
        << Color::BRIGHT_CYAN << "Payment Received   : RM" << Color::BRIGHT_WHITE << paymentReceived << "\n"
        << Color::BRIGHT_CYAN << "Total SST Collected: RM" << Color::BRIGHT_WHITE << totalTax << "\n"
        << Color::BRIGHT_CYAN << "Outstanding Amount : RM" << Color::BRIGHT_WHITE << outstandingAmount << "\n"
        << Color::BRIGHT_CYAN << "Completed Services : " << Color::BRIGHT_WHITE << completedServicesCount << "\n"
        << Color::BRIGHT_CYAN << "Unpaid Bills       : " << Color::BRIGHT_WHITE << unpaidBills << "\n"
        << Color::BRIGHT_CYAN << "Average Daily Sales: RM" << Color::BRIGHT_WHITE << avgDailySales << "\n\n";

    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "--- PAYMENT BREAKDOWN STATISTICS ---\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << std::left << std::setw(15) << "Method"
        << std::setw(15) << "Count"
        << "Amount (RM)\n" << Color::RESET;
    std::cout << Color::CYAN << "-----------------------------------------\n" << Color::RESET;
    std::cout << Color::BRIGHT_WHITE << std::left << std::setw(15) << "Cash"
        << std::setw(15) << cashPayments
        << std::fixed << std::setprecision(2) << cashAmount << "\n";
    std::cout << Color::BRIGHT_WHITE << std::left << std::setw(15) << "Card"
        << std::setw(15) << cardPayments
        << cardAmount << "\n";
    std::cout << Color::BRIGHT_WHITE << std::left << std::setw(15) << "E-Wallet"
        << std::setw(15) << ewalletPayments
        << ewalletAmount << "\n";
    std::cout << Color::CYAN << "========================================\n" << Color::RESET;

    pauseScreen();
}

void yearlyReport(const std::vector<Invoice>& invoices) {
    if (invoices.empty()) {
        std::cout << Color::BRIGHT_YELLOW << "\n[INFO] No invoice data available.\n" << Color::RESET;
        pauseScreen();
        return;
    }

    int year = 0;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Year (YYYY): " << Color::RESET;
    if (!(std::cin >> year) || year <= 0) {
        std::cout << Color::BRIGHT_RED << "ERROR: Invalid year input.\n" << Color::RESET;
        clearCinError();
        pauseScreen();
        return;
    }
    std::cin.ignore();

    std::array<double, 12> monthlyTotals{};
    monthlyTotals.fill(0.0);
    double totalYearlySales = 0.0;
    double totalYearlyTax = 0.0;
    int totalBills = 0;

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        if (d.year != year) continue;
        monthlyTotals[d.month - 1] += inv.totalAmount;
        totalYearlySales += inv.totalAmount;
        totalYearlyTax += inv.taxAmount;
        totalBills++;
    }

    static const char* monthNames[12] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
        << Color::BRIGHT_YELLOW << Color::BOLD << "          YEARLY SALES REPORT           \n" << Color::RESET
        << Color::CYAN << "========================================\n" << Color::RESET
        << Color::BRIGHT_CYAN << "Year: " << Color::BRIGHT_WHITE << year << "\n"
        << Color::BRIGHT_CYAN << "Total Bills: " << Color::BRIGHT_WHITE << totalBills << "\n\n" << Color::RESET;

    std::cout << std::fixed << std::setprecision(2);
    for (int i = 0; i < 12; ++i) {
        std::cout << Color::BRIGHT_CYAN << std::left << std::setw(15) << monthNames[i]
            << Color::BRIGHT_WHITE << "RM" << monthlyTotals[i] << "\n" << Color::RESET;
    }
    std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET
        << Color::BRIGHT_YELLOW << "Total Yearly Sales: RM" << Color::BRIGHT_WHITE << totalYearlySales << "\n"
        << Color::BRIGHT_CYAN << "Total SST Collected: RM" << Color::BRIGHT_WHITE << totalYearlyTax << "\n"
        << Color::CYAN << "========================================\n" << Color::RESET;

    pauseScreen();
}

// ===========================================================================
// FIXED SALES ANALYSIS MENU - USING getline FOR INPUT
// ===========================================================================
void salesAnalysisMenu(const std::vector<Invoice>& invoices) {
    int choice = -1;
    std::string input;

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

        // Use getline to properly handle input
        std::getline(std::cin, input);

        // If input is empty (due to previous newline), read again
        if (input.empty()) {
            std::getline(std::cin, input);
        }

        // Convert to integer
        try {
            choice = std::stoi(input);
        }
        catch (...) {
            std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
            pauseScreen();
            continue;
        }

        if (choice < 0 || choice > 5) {
            std::cout << Color::BRIGHT_RED << "Invalid choice! Please select 0-5.\n" << Color::RESET;
            pauseScreen();
            continue;
        }

        if (choice == 0) break;

        if (invoices.empty()) {
            std::cout << Color::BRIGHT_RED << "\nERROR: No invoice data available.\n"
                << Color::BRIGHT_YELLOW << "Please add invoices first.\n" << Color::RESET;
            pauseScreen();
            continue;
        }

        // Process data
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
        int parsedCount = 0;

        for (const auto& inv : invoices) {
            ParsedDate d;
            if (!parseDate(inv.date, d)) {
                continue;
            }
            parsedCount++;
            findOrAddYear(d.year)[d.month - 1] += inv.totalAmount;
            totalSales += inv.totalAmount;
        }

        if (parsedCount == 0) {
            std::cout << Color::BRIGHT_RED << "\nERROR: No invoices with valid dates found.\n"
                << Color::BRIGHT_YELLOW << "Please check your invoice dates.\n" << Color::RESET;
            pauseScreen();
            continue;
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

        // ============= DISPLAY RESULTS =============
        std::cout << "\n\n";
        std::cout << Color::BRIGHT_GREEN << "========================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "             RESULT                    \n" << Color::RESET;
        std::cout << Color::BRIGHT_GREEN << "========================================\n" << Color::RESET;

        switch (choice) {
        case 1: { // Best Sales Month
            if (entries.empty()) {
                std::cout << Color::BRIGHT_YELLOW << "No sales data available.\n" << Color::RESET;
                break;
            }
            auto best = std::max_element(entries.begin(), entries.end(),
                [](const MonthEntry& a, const MonthEntry& b) { return a.total < b.total; });
            std::cout << Color::BRIGHT_CYAN << "Best Sales Month : " << Color::BRIGHT_WHITE << monthNames[best->month] << " " << best->year << "\n";
            std::cout << Color::BRIGHT_CYAN << "Total Sales      : " << Color::BRIGHT_WHITE << "RM" << std::fixed << std::setprecision(2) << best->total << "\n";
            break;
        }
        case 2: { // Lowest Sales Month
            if (entries.empty()) {
                std::cout << Color::BRIGHT_YELLOW << "No sales data available.\n" << Color::RESET;
                break;
            }
            auto worst = std::min_element(entries.begin(), entries.end(),
                [](const MonthEntry& a, const MonthEntry& b) { return a.total < b.total; });
            std::cout << Color::BRIGHT_CYAN << "Lowest Sales Month: " << Color::BRIGHT_WHITE << monthNames[worst->month] << " " << worst->year << "\n";
            std::cout << Color::BRIGHT_CYAN << "Total Sales      : " << Color::BRIGHT_WHITE << "RM" << std::fixed << std::setprecision(2) << worst->total << "\n";
            break;
        }
        case 3: { // Average Monthly Sales
            int activeMonths = static_cast<int>(entries.size());
            double avg = activeMonths > 0 ? (totalSales / activeMonths) : 0.0;
            std::cout << Color::BRIGHT_CYAN << "Average Monthly Sales: " << Color::BRIGHT_WHITE << "RM" << std::fixed << std::setprecision(2) << avg << "\n";
            std::cout << Color::LIGHT_GRAY << "  (Based on " << activeMonths << " months with sales data)\n";
            break;
        }
        case 4: { // Total Yearly Sales
            std::cout << Color::BRIGHT_CYAN << "Total Sales (All Recorded): " << Color::BRIGHT_WHITE << "RM" << std::fixed << std::setprecision(2) << totalSales << "\n";
            std::cout << Color::LIGHT_GRAY << "  (Based on " << parsedCount << " invoices)\n";
            break;
        }
        case 5: { // Sales Growth
            if (years.size() < 2) {
                std::cout << Color::BRIGHT_YELLOW << "Not enough historical data (need at least 2 distinct years).\n";
                std::cout << Color::LIGHT_GRAY << "  Found years: ";
                for (int y : years) std::cout << y << " ";
                std::cout << "\n";
            }
            else {
                std::vector<int> sortedYears = years;
                std::sort(sortedYears.begin(), sortedYears.end());
                int latestYear = sortedYears.back();
                int prevYear = sortedYears[sortedYears.size() - 2];

                double latestTotal = 0.0, prevTotal = 0.0;
                for (size_t i = 0; i < years.size(); ++i) {
                    if (years[i] == latestYear) {
                        for (double v : perYearTotals[i]) latestTotal += v;
                    }
                    if (years[i] == prevYear) {
                        for (double v : perYearTotals[i]) prevTotal += v;
                    }
                }

                if (prevTotal <= 0.0) {
                    std::cout << Color::BRIGHT_YELLOW << "Cannot compute (previous year " << prevYear << " has no recorded sales).\n";
                }
                else {
                    double growthPct = ((latestTotal - prevTotal) / prevTotal) * 100.0;
                    std::cout << Color::BRIGHT_CYAN << "Sales Growth (" << prevYear << " -> " << latestYear << "): "
                        << Color::BRIGHT_WHITE << std::fixed << std::setprecision(1) << std::showpos << growthPct << std::noshowpos << "%\n";
                    std::cout << Color::LIGHT_GRAY << "  " << prevYear << " total: RM" << std::fixed << std::setprecision(2) << prevTotal << "\n";
                    std::cout << Color::LIGHT_GRAY << "  " << latestYear << " total: RM" << std::fixed << std::setprecision(2) << latestTotal << "\n";
                }
            }
            break;
        }
        }

        std::cout << Color::BRIGHT_GREEN << "========================================\n" << Color::RESET;

        // ============= PAUSE SO USER CAN SEE RESULTS =============
        pauseScreen();

    } while (choice != 0);
}

// ===========================================================================
// END OF SALES ANALYSIS MENU
// ===========================================================================

void exportReports(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    if (invoices.empty()) {
        std::cout << Color::BRIGHT_YELLOW << "\n[INFO] No invoice data available to export.\n" << Color::RESET;
        pauseScreen();
        return;
    }

    int month = 0, year = 0;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Month (1-12) for export: " << Color::RESET;
    if (!(std::cin >> month) || month < 1 || month > 12) {
        std::cout << Color::BRIGHT_RED << "ERROR: Invalid month entered. Enter values 1-12.\n" << Color::RESET;
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
    std::cin.ignore();

    std::string filename = "sales_" + std::to_string(month) + "_" + std::to_string(year) + ".txt";

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cout << Color::BRIGHT_RED << "[ERROR] Failed to write report file.\n" << Color::RESET;
        pauseScreen();
        return;
    }

    outFile << std::fixed << std::setprecision(2)
        << "=========================================================\n"
        << "        CAR WORKSHOP - MONTHLY REPORT EXPORT            \n"
        << "=========================================================\n\n"
        << "Report Month: " << month << "/" << year << "\n"
        << "Generated On: " << getCurrentDateString() << "\n\n";

    double totalSales = 0.0, paymentReceived = 0.0, outstandingAmount = 0.0;
    double totalTax = 0.0;
    int totalBills = 0, paidBills = 0;
    int cashPayments = 0, cardPayments = 0, ewalletPayments = 0;
    double cashAmount = 0.0, cardAmount = 0.0, ewalletAmount = 0.0;

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        if (d.month != month || d.year != year) continue;

        totalBills++;
        totalSales += inv.totalAmount;
        totalTax += inv.taxAmount;

        if (inv.isPaid) {
            paymentReceived += inv.totalAmount;
            paidBills++;

            if (inv.paymentType == "Cash") {
                cashPayments++;
                cashAmount += inv.totalAmount;
            }
            else if (inv.paymentType == "Card") {
                cardPayments++;
                cardAmount += inv.totalAmount;
            }
            else if (inv.paymentType == "E-Wallet") {
                ewalletPayments++;
                ewalletAmount += inv.totalAmount;
            }
        }
        else {
            outstandingAmount += inv.totalAmount;
        }
    }

    outFile << "-- FINANCIAL SUMMARY --\n"
        << "Total Bills        : " << totalBills << "\n"
        << "Paid Bills         : " << paidBills << "\n"
        << "Total Sales        : RM" << totalSales << "\n"
        << "Payment Received   : RM" << paymentReceived << "\n"
        << "Outstanding Amount : RM" << outstandingAmount << "\n"
        << "Total SST Collected: RM" << totalTax << "\n\n";

    outFile << "-- PAYMENT BREAKDOWN --\n"
        << std::left << std::setw(15) << "Method"
        << std::setw(15) << "Count"
        << "Amount (RM)\n"
        << std::left << std::setw(15) << "Cash"
        << std::setw(15) << cashPayments
        << cashAmount << "\n"
        << std::left << std::setw(15) << "Card"
        << std::setw(15) << cardPayments
        << cardAmount << "\n"
        << std::left << std::setw(15) << "E-Wallet"
        << std::setw(15) << ewalletPayments
        << ewalletAmount << "\n\n";

    // Customer statistics
    int statsMatrix[4][2] = { {0} };
    double totalSpending[4] = { 0.0 };
    for (const auto& c : customers) {
        int rowIdx = 0;
        if (c.membershipTier == "Silver") rowIdx = 1;
        else if (c.membershipTier == "Gold") rowIdx = 2;
        else if (c.membershipTier == "VIP") rowIdx = 3;
        statsMatrix[rowIdx][0]++;
        statsMatrix[rowIdx][1] += static_cast<int>(c.vehicles.size());
        totalSpending[rowIdx] += c.totalPurchases;
    }

    outFile << "-- CUSTOMER STATISTICS --\n"
        << std::left << std::setw(15) << "Tier"
        << std::setw(18) << "Total Customers"
        << std::setw(15) << "Total Cars"
        << "Total Spending (RM)\n"
        << std::left << std::setw(15) << "Regular"
        << std::setw(18) << statsMatrix[0][0]
        << std::setw(15) << statsMatrix[0][1]
        << totalSpending[0] << "\n"
        << std::left << std::setw(15) << "Silver"
        << std::setw(18) << statsMatrix[1][0]
        << std::setw(15) << statsMatrix[1][1]
        << totalSpending[1] << "\n"
        << std::left << std::setw(15) << "Gold"
        << std::setw(18) << statsMatrix[2][0]
        << std::setw(15) << statsMatrix[2][1]
        << totalSpending[2] << "\n"
        << std::left << std::setw(15) << "VIP"
        << std::setw(18) << statsMatrix[3][0]
        << std::setw(15) << statsMatrix[3][1]
        << totalSpending[3] << "\n\n";

    outFile << "-- ITEMIZED INVOICE LIST --\n"
        << std::left << std::setw(12) << "Bill ID"
        << std::setw(25) << "Customer"
        << std::setw(12) << "Plate"
        << std::setw(14) << "Amount (RM)"
        << "Status\n";

    for (const auto& inv : invoices) {
        ParsedDate d;
        if (!parseDate(inv.date, d)) continue;
        if (d.month != month || d.year != year) continue;

        std::string custName = getCustomerName(customers, inv.customerId);
        outFile << std::left << std::setw(12) << inv.invoiceId
            << std::setw(25) << custName
            << std::setw(12) << inv.licensePlate
            << std::setw(14) << inv.totalAmount
            << (inv.isPaid ? "PAID" : "UNPAID") << "\n";
    }

    outFile << "=========================================================\n";
    outFile.close();

    std::cout << Color::BRIGHT_GREEN << "[SUCCESS] Report saved to " << Color::BRIGHT_WHITE << filename << "\n" << Color::RESET;
    pauseScreen();
}