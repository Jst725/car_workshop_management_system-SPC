#include "billing.h"
#include "appointment.h"
#include "service.h"
#include "customer.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <ctime>
#include <regex>
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

// Helper function to clear input buffer errors
static void clearCinError() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Function to clear the console screen cross-platform
static void clearScreen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

// Cross-platform pause function to prompt user before screen clear or transition
static void pauseSystem() {
    std::cout << Color::BRIGHT_YELLOW << "\nPress Enter to continue..." << Color::RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

static std::string toUpper(std::string str) {
    for (char& c : str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return str;
}

static std::string shortenText(const std::string& str, size_t maxLen = 30) {
    if (str.length() <= maxLen) return str;
    return str.substr(0, maxLen - 3) + "...";
}

static std::string getCurrentDate() {
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

static double getTierDiscountRate(const std::string& tier) {
    if (tier == "Silver") return 0.05;
    if (tier == "Gold")   return 0.10;
    if (tier == "VIP")    return 0.20;
    return 0.0;
}

static void updateCustomerTier(Customer& customer) {
    if (customer.totalPurchases >= 5000.0) customer.membershipTier = "VIP";
    else if (customer.totalPurchases >= 2000.0) customer.membershipTier = "Gold";
    else if (customer.totalPurchases >= 800.0)  customer.membershipTier = "Silver";
    else customer.membershipTier = "Regular";
}

static std::string generateNextInvoiceId(const std::vector<Invoice>& invoices) {
    int maxNum = 0;
    for (const auto& inv : invoices) {
        std::string digits;
        for (char c : inv.invoiceId) {
            if (std::isdigit(static_cast<unsigned char>(c))) digits += c;
        }
        if (!digits.empty()) {
            int n = std::stoi(digits);
            if (n > maxNum) maxNum = n;
        }
    }
    return "B00" + std::to_string(maxNum + 1);
}

static void printInvoiceDetails(const Invoice& inv, const std::string& customerName) {
    std::cout << Color::CYAN << "\n========================================================================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "                              OFFICIAL WORKSHOP INVOICE                                 \n" << Color::RESET;
    std::cout << Color::CYAN << "========================================================================================\n" << Color::RESET;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << Color::BRIGHT_CYAN << "Bill ID         : " << Color::BRIGHT_WHITE << inv.invoiceId << "\n";
    std::cout << Color::BRIGHT_CYAN << "Customer        : " << Color::BRIGHT_WHITE << customerName << " (Phone: " << inv.customerId << ")\n";
    std::cout << Color::BRIGHT_CYAN << "License Plate   : " << Color::BRIGHT_WHITE << inv.licensePlate << "\n";
    std::cout << Color::BRIGHT_CYAN << "Invoice Date    : " << Color::BRIGHT_WHITE << inv.date << "\n";
    std::cout << Color::CYAN << "----------------------------------------------------------------------------------------\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << std::left << std::setw(12) << "Type"
        << std::setw(34) << "Description"
        << std::setw(8) << "Qty"
        << std::setw(18) << "Unit Price(RM)"
        << "Total Price(RM)\n" << Color::RESET;
    std::cout << Color::CYAN << "----------------------------------------------------------------------------------------\n" << Color::RESET;

    if (inv.items.empty()) {
        double defaultUnit = inv.partsCost + inv.laborCost;
        std::cout << Color::BRIGHT_WHITE << std::left << std::setw(12) << "Service"
            << std::setw(34) << shortenText("General Service & Repair", 32)
            << std::setw(8) << 1
            << std::setw(18) << defaultUnit
            << defaultUnit << "\n" << Color::RESET;
    }
    else {
        for (const auto& item : inv.items) {
            std::cout << Color::BRIGHT_WHITE << std::left << std::setw(12) << item.itemType
                << std::setw(34) << shortenText(item.description, 32)
                << std::setw(8) << item.quantity
                << std::setw(18) << item.unitPrice
                << item.totalPrice << "\n" << Color::RESET;
        }
    }

    double subtotal = inv.partsCost + inv.laborCost;
    std::cout << Color::CYAN << "----------------------------------------------------------------------------------------\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << "Subtotal (Parts: RM" << inv.partsCost << " | Labor: RM" << inv.laborCost << "): RM" << subtotal << "\n";
    std::cout << Color::BRIGHT_CYAN << "Discount Amount : -RM" << inv.discountAmount << "\n";
    std::cout << Color::BRIGHT_CYAN << "Service Tax (6%): RM" << inv.taxAmount << "\n";
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "TOTAL AMOUNT    : RM" << inv.totalAmount << "\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << "Payment Status  : " << (inv.isPaid ? (Color::BRIGHT_CYAN + "PAID" + Color::RESET) : (Color::BRIGHT_RED + "UNPAID" + Color::RESET)) << "\n";
    std::cout << Color::CYAN << "========================================================================================\n" << Color::RESET;
}

// Validate card transaction reference (must be exactly 12 digits)
static bool validateCardTransaction(const std::string& ref) {
    if (ref.length() != 12) return false;
    for (char c : ref) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

// Display strictly unpaid invoices, with an option to select and view detailed invoice slip
void displayInvoices(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers) {
    std::vector<const Invoice*> unpaidInvoices;
    for (const auto& inv : invoices) {
        if (!inv.isPaid) {
            unpaidInvoices.push_back(&inv);
        }
    }

    if (unpaidInvoices.empty()) {
        std::cout << Color::BRIGHT_YELLOW << "\nNo unpaid bill records found in storage.\n" << Color::RESET;
        return;
    }

    std::cout << Color::CYAN << "\n------------------------------------------------------------------------------------------------------------------\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << std::left << std::setw(6) << "No."
        << std::setw(10) << "Bill ID"
        << std::setw(15) << "Phone Number"
        << std::setw(12) << "Plate"
        << std::setw(12) << "Subtotal"
        << std::setw(12) << "Discount"
        << std::setw(12) << "Tax(6%)"
        << std::setw(12) << "Total(RM)"
        << std::setw(12) << "Method"
        << "Status\n" << Color::RESET;
    std::cout << Color::CYAN << "------------------------------------------------------------------------------------------------------------------\n" << Color::RESET;

    for (size_t i = 0; i < unpaidInvoices.size(); ++i) {
        const auto& inv = *unpaidInvoices[i];
        double sub = inv.partsCost + inv.laborCost;
        std::cout << Color::BRIGHT_WHITE << std::left << std::setw(6) << (i + 1)
            << std::setw(10) << inv.invoiceId
            << std::setw(15) << inv.customerId
            << std::setw(12) << inv.licensePlate
            << std::setw(12) << std::fixed << std::setprecision(2) << sub
            << std::setw(12) << inv.discountAmount
            << std::setw(12) << inv.taxAmount
            << std::setw(12) << inv.totalAmount
            << std::setw(12) << inv.paymentType
            << (Color::BRIGHT_RED + "Unpaid" + Color::RESET) << "\n";
    }
    std::cout << Color::CYAN << "------------------------------------------------------------------------------------------------------------------\n" << Color::RESET;

    std::cout << Color::BRIGHT_YELLOW << "\nWould you like to view invoice details (slip)? (Y/N): " << Color::RESET;
    char viewChoice;
    std::cin >> viewChoice;
    if (std::toupper(viewChoice) == 'Y') {
        int idx;
        std::cout << Color::BRIGHT_YELLOW << "Enter Index No. (1-" << unpaidInvoices.size() << "): " << Color::RESET;
        if (std::cin >> idx && idx >= 1 && idx <= static_cast<int>(unpaidInvoices.size())) {
            const Invoice* selected = unpaidInvoices[idx - 1];
            std::string custName = "Unknown";
            auto cIt = std::find_if(customers.begin(), customers.end(), [&](const Customer& c) {
                return c.phone == selected->customerId;
                });
            if (cIt != customers.end()) custName = cIt->name;
            printInvoiceDetails(*selected, custName);
        }
        else {
            std::cout << Color::BRIGHT_RED << "Invalid index selection.\n" << Color::RESET;
            clearCinError();
        }
    }
}

// Function to search BOTH PAID & UNPAID invoices by Phone Number, blocking edits on paid ones
void searchInvoiceByPhone(std::vector<Invoice>& invoices, std::vector<Customer>& customers) {
    std::string phoneInput;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Customer Phone Number to search: " << Color::RESET;
    std::cin >> phoneInput;

    std::string searchKey = toUpper(phoneInput);
    std::vector<Invoice*> allMatches;

    for (auto& inv : invoices) {
        if (toUpper(inv.customerId) == searchKey) {
            allMatches.push_back(&inv);
        }
    }

    std::cout << Color::CYAN << "\n========================================================================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "                      INVOICE SEARCH RESULTS (Phone: " << phoneInput << ")                      \n" << Color::RESET;
    std::cout << Color::CYAN << "========================================================================================\n" << Color::RESET;

    if (allMatches.empty()) {
        std::cout << Color::BRIGHT_RED << "No invoices found for Customer Phone Number: [" << phoneInput << "]\n" << Color::RESET;
        std::cout << Color::CYAN << "========================================================================================\n" << Color::RESET;
        return;
    }

    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << std::left << std::setw(6) << "No."
        << std::setw(12) << "Bill ID"
        << std::setw(15) << "Phone Number"
        << std::setw(14) << "Plate"
        << std::setw(14) << "Date"
        << std::setw(15) << "Total (RM)"
        << std::setw(12) << "Method"
        << "Status\n" << Color::RESET;
    std::cout << Color::CYAN << "----------------------------------------------------------------------------------------\n" << Color::RESET;

    for (size_t i = 0; i < allMatches.size(); ++i) {
        const auto& inv = *allMatches[i];
        std::cout << Color::BRIGHT_WHITE << std::left << std::setw(6) << (i + 1)
            << std::setw(12) << inv.invoiceId
            << std::setw(15) << inv.customerId
            << std::setw(14) << inv.licensePlate
            << std::setw(14) << inv.date
            << std::setw(15) << std::fixed << std::setprecision(2) << inv.totalAmount
            << std::setw(12) << inv.paymentType
            << (inv.isPaid ? (Color::BRIGHT_CYAN + "PAID" + Color::RESET) : (Color::BRIGHT_RED + "UNPAID" + Color::RESET)) << "\n";
    }
    std::cout << Color::CYAN << "========================================================================================\n" << Color::RESET;

    std::cout << Color::BRIGHT_CYAN << "\nActions:\n";
    std::cout << "1. View Invoice Details (Slip)\n";
    std::cout << "2. Edit Invoice\n";
    std::cout << "0. Return\n";
    std::cout << Color::BRIGHT_YELLOW << "Choose action: " << Color::RESET;

    int actChoice;
    if (!(std::cin >> actChoice) || actChoice == 0) {
        if (actChoice != 0) clearCinError();
        return;
    }

    if (actChoice == 1 || actChoice == 2) {
        int idx;
        std::cout << Color::BRIGHT_YELLOW << "Select Invoice No. (1-" << allMatches.size() << "): " << Color::RESET;
        if (std::cin >> idx && idx >= 1 && idx <= static_cast<int>(allMatches.size())) {
            Invoice* target = allMatches[idx - 1];
            if (actChoice == 1) {
                std::string custName = "Unknown";
                auto cIt = std::find_if(customers.begin(), customers.end(), [&](const Customer& c) {
                    return c.phone == target->customerId;
                    });
                if (cIt != customers.end()) custName = cIt->name;
                printInvoiceDetails(*target, custName);
            }
            else {
                // Check if invoice is paid before allowing edit
                if (target->isPaid) {
                    std::cout << Color::BRIGHT_RED << "\nERROR: Invoice [" << target->invoiceId << "] is already PAID and CANNOT be edited!\n" << Color::RESET;
                    return;
                }

                // Perform Edit on the selected UNPAID invoice directly
                std::cout << Color::CYAN << "\n--- EDIT UNPAID INVOICE [" << target->invoiceId << "] ---\n" << Color::RESET;
                std::cout << Color::BRIGHT_CYAN << "1. Update Bill Date\n";
                std::cout << "2. Apply / Change Discount Rate\n";
                std::cout << "3. Delete Bill\n";
                std::cout << Color::BRIGHT_RED << "0. Cancel\n" << Color::RESET;
                std::cout << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;
                int uc;
                if (!(std::cin >> uc) || uc == 0) { clearCinError(); return; }

                if (uc == 1) {
                    std::cout << Color::BRIGHT_YELLOW << "Enter new date (YYYY-MM-DD): " << Color::RESET;
                    std::cin >> target->date;
                    saveInvoicesToFile(invoices);
                    std::cout << Color::BRIGHT_CYAN << "Bill date updated and saved successfully.\n" << Color::RESET;
                }
                else if (uc == 2) {
                    std::cout << Color::BRIGHT_YELLOW << "\nChoose Discount Option:\n" << Color::RESET;
                    std::cout << Color::BRIGHT_CYAN << "1. Apply Tier-Based Discount (Auto via Customer Membership)\n";
                    std::cout << "2. Apply Custom Percentage Discount\n";
                    std::cout << "3. Apply Custom Fixed Amount Discount (RM)\n";
                    std::cout << Color::BRIGHT_RED << "0. Cancel\n" << Color::RESET;
                    std::cout << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;

                    int dChoice;
                    if (!(std::cin >> dChoice) || dChoice == 0) { clearCinError(); return; }

                    double sub = target->partsCost + target->laborCost;
                    double discAmount = 0.0;
                    double discPercentageShown = 0.0;

                    if (dChoice == 1) {
                        double discRate = 0.0;
                        auto custIt = std::find_if(customers.begin(), customers.end(), [&](const Customer& c) {
                            return c.phone == target->customerId;
                            });
                        if (custIt != customers.end()) {
                            discRate = getTierDiscountRate(custIt->membershipTier);
                        }
                        discAmount = sub * discRate;
                        discPercentageShown = discRate * 100.0;
                    }
                    else if (dChoice == 2) {
                        double pct = 0.0;
                        std::cout << Color::BRIGHT_YELLOW << "Enter discount percentage (e.g., 15 for 15%): " << Color::RESET;
                        std::cin >> pct;
                        if (pct < 0 || pct > 100) {
                            std::cout << Color::BRIGHT_RED << "Invalid percentage range!\n" << Color::RESET;
                            return;
                        }
                        discAmount = sub * (pct / 100.0);
                        discPercentageShown = pct;
                    }
                    else if (dChoice == 3) {
                        std::cout << Color::BRIGHT_YELLOW << "Enter fixed discount amount (RM): " << Color::RESET;
                        std::cin >> discAmount;
                        if (discAmount < 0) discAmount = 0;
                        if (discAmount > sub) discAmount = sub;
                        discPercentageShown = (sub > 0) ? (discAmount / sub) * 100.0 : 0.0;
                    }

                    target->discountAmount = discAmount;
                    target->taxAmount = (sub - target->discountAmount) * 0.06;
                    target->totalAmount = (sub - target->discountAmount) + target->taxAmount;

                    saveInvoicesToFile(invoices);

                    std::cout << std::fixed << std::setprecision(2);
                    std::cout << Color::BRIGHT_CYAN << "Discount applied (" << discPercentageShown << "% equivalent / RM" << target->discountAmount << " off).\n";
                    std::cout << "New Total (incl. 6% Tax): RM" << target->totalAmount << "\n";
                    std::cout << "Changes stored to file successfully.\n" << Color::RESET;
                }
                else if (uc == 3) {
                    std::cout << Color::BRIGHT_YELLOW << "Confirm Delete? (Y/N): " << Color::RESET;
                    char confirm;
                    std::cin >> confirm;
                    if (std::toupper(confirm) == 'Y') {
                        auto eraseIt = std::find_if(invoices.begin(), invoices.end(), [&](const Invoice& inv) {
                            return inv.invoiceId == target->invoiceId;
                            });
                        if (eraseIt != invoices.end()) {
                            invoices.erase(eraseIt);
                        }
                        saveInvoicesToFile(invoices);
                        std::cout << Color::BRIGHT_CYAN << "Bill deleted and changes stored to file successfully.\n" << Color::RESET;
                    }
                }
            }
        }
        else {
            std::cout << Color::BRIGHT_RED << "Invalid selection!\n" << Color::RESET;
            clearCinError();
        }
    }
}

void billingMenu(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments) {
    int choice = -1;
    do {
        clearScreen();
        std::cout << Color::CYAN << "\n========================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "        BILL & PAYMENT MANAGEMENT        \n" << Color::RESET;
        std::cout << Color::CYAN << "========================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Generate Invoice\n";
        std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "View Unpaid Invoices\n";
        std::cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Search Invoice (by Phone Number)\n";
        std::cout << Color::BRIGHT_CYAN << "4. " << Color::BRIGHT_WHITE << "Edit Invoice (Unpaid Only)\n";
        std::cout << Color::BRIGHT_CYAN << "5. " << Color::BRIGHT_WHITE << "Process Payment (Print Receipt)\n";
        std::cout << Color::BRIGHT_CYAN << "6. " << Color::BRIGHT_WHITE << "Payment History\n";
        std::cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Back to Main Menu\n";
        std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << "Enter your choice: " << Color::RESET;

        if (!(std::cin >> choice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
            clearCinError();
            pauseSystem();
            continue;
        }

        switch (choice) {
        case 1:
            clearScreen();
            generateInvoice(invoices, customers, appointments);
            pauseSystem();
            break;
        case 2:
            clearScreen();
            displayInvoices(invoices, customers);
            pauseSystem();
            break;
        case 3:
            clearScreen();
            searchInvoiceByPhone(invoices, customers);
            pauseSystem();
            break;
        case 4:
            clearScreen();
            editInvoice(invoices, customers);
            pauseSystem();
            break;
        case 5:
            clearScreen();
            processPayment(invoices, customers, appointments);
            pauseSystem();
            break;
        case 6:
            clearScreen();
            paymentHistory(customers, invoices);
            break;
        case 0:
            std::cout << Color::BRIGHT_CYAN << "Returning to Main Menu...\n" << Color::RESET;
            break;
        default:
            std::cout << Color::BRIGHT_RED << "Invalid choice! Please select an option between 0 and 6.\n" << Color::RESET;
            pauseSystem();
        }
    } while (choice != 0);
}

void generateInvoice(std::vector<Invoice>& invoices, std::vector<Customer>& customers, const std::vector<Appointment>& /*appointments*/) {
    std::string phoneInput;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Customer Phone Number: " << Color::RESET;
    std::cin >> phoneInput;

    auto custIt = std::find_if(customers.begin(), customers.end(), [&](Customer& c) {
        return c.phone == phoneInput;
        });

    if (custIt == customers.end()) {
        std::cout << Color::BRIGHT_RED << "ERROR: Customer with phone [" << phoneInput << "] not found in system!\n" << Color::RESET;
        return;
    }

    Customer& targetCustomer = *custIt;

    std::cout << Color::CYAN << "\n========================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "        SELECT COMPLETED SERVICE        \n" << Color::RESET;
    std::cout << Color::CYAN << "========================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << "Customer: " << Color::BRIGHT_WHITE << targetCustomer.name << "\n\n" << Color::RESET;

    auto hasPendingInvoice = [&](const std::string& plate) {
        for (const auto& existing : invoices) {
            if (!existing.isPaid && existing.licensePlate == plate) return true;
        }
        return false;
        };

    bool foundValidService = false;
    bool skippedAlreadyInvoiced = false;
    int indexCounter = 1;
    for (auto& v : targetCustomer.vehicles) {
        if (v.currentServiceCost > 0.0 && v.status == "Ready for Pickup") {
            if (hasPendingInvoice(v.licensePlate)) {
                skippedAlreadyInvoiced = true;
                continue;
            }
            foundValidService = true;
            std::string descDisplay = !v.lastPartsUsed.empty() ? v.lastPartsUsed :
                (!v.lastServiceDescription.empty() ? v.lastServiceDescription : "N/A");

            std::cout << Color::BRIGHT_CYAN << indexCounter << ". " << Color::BRIGHT_WHITE << "S00" << indexCounter << " | " << v.licensePlate
                << " | Service: " << shortenText(descDisplay, 25)
                << " | Parts Cost: RM" << std::fixed << std::setprecision(2) << v.partsCost
                << " | Labor: RM" << v.laborCost
                << " | Total: RM" << v.currentServiceCost << "\n" << Color::RESET;
            indexCounter++;
        }
    }

    if (!foundValidService) {
        if (skippedAlreadyInvoiced) {
            std::cout << Color::BRIGHT_YELLOW << "INFO: This customer's completed service(s) already have an unpaid invoice on file.\n";
            std::cout << "      Use \"Process Payment\" to settle it instead of generating a new one.\n" << Color::RESET;
        }
        else {
            std::cout << Color::BRIGHT_YELLOW << "INFO: No completed services with pending bills found for this customer.\n" << Color::RESET;
        }
        return;
    }

    int choiceIndex;
    std::cout << Color::BRIGHT_YELLOW << "\nSelect Service: " << Color::RESET;
    if (!(std::cin >> choiceIndex)) {
        std::cout << Color::BRIGHT_RED << "Invalid selection.\n" << Color::RESET;
        clearCinError();
        return;
    }

    Vehicle* targetVehicle = nullptr;
    int currentIdx = 1;
    for (auto& v : targetCustomer.vehicles) {
        if (v.currentServiceCost > 0.0 && v.status == "Ready for Pickup" && !hasPendingInvoice(v.licensePlate)) {
            if (currentIdx == choiceIndex) {
                targetVehicle = &v;
                break;
            }
            currentIdx++;
        }
    }

    if (!targetVehicle) {
        std::cout << Color::BRIGHT_RED << "ERROR: Invalid selection index!\n" << Color::RESET;
        return;
    }

    Invoice inv;
    inv.invoiceId = generateNextInvoiceId(invoices);
    inv.customerId = targetCustomer.phone;
    inv.licensePlate = targetVehicle->licensePlate;
    inv.date = targetVehicle->lastServiceDate.empty() ? getCurrentDate() : targetVehicle->lastServiceDate;

    inv.partsCost = targetVehicle->partsCost;
    inv.laborCost = targetVehicle->laborCost;

    // 1. If structured parts exist in vehicle vector, add them directly
    if (!targetVehicle->parts.empty()) {
        for (const auto& p : targetVehicle->parts) {
            InvoiceItem item;
            item.description = p.name;
            item.itemType = p.isFromAppointment ? "Service" : "Part";
            item.quantity = p.quantity;
            item.unitPrice = p.unitPrice;
            item.totalPrice = p.price;
            inv.items.push_back(item);
        }
    }
    // 2. Parse appointment string format
    else {
        std::string rawDesc = !targetVehicle->lastPartsUsed.empty() ? targetVehicle->lastPartsUsed :
            (!targetVehicle->lastServiceDescription.empty() ? targetVehicle->lastServiceDescription : "");

        rawDesc = std::regex_replace(rawDesc, std::regex(R"(\{Total Service Fee:[^\}]*\})"), "");

        std::stringstream ss(rawDesc);
        std::string segment;
        std::vector<InvoiceItem> parsedItems;

        while (std::getline(ss, segment, '|')) {
            size_t start = segment.find_first_not_of(" \t\r\n");
            if (start != std::string::npos) segment = segment.substr(start);
            size_t end = segment.find_last_not_of(" \t\r\n");
            if (end != std::string::npos) segment = segment.substr(0, end + 1);

            if (segment.empty()) continue;

            std::smatch match;
            std::regex priceRegex(R"(\[\s*[\$|RM]*\s*([0-9]+(?:\.[0-9]+)?)\s*\])");

            double itemPrice = 0.0;
            if (std::regex_search(segment, match, priceRegex)) {
                itemPrice = std::stod(match[1].str());
            }

            std::string cleanName = std::regex_replace(segment, std::regex(R"(\(\d+\)\s*\[.*?\]\s*->?\s*)"), "");
            cleanName = std::regex_replace(cleanName, priceRegex, "");
            cleanName = std::regex_replace(cleanName, std::regex(R"(\[.*?\])"), "");

            size_t nameEnd = cleanName.find_last_not_of(" \t\r\n");
            if (nameEnd != std::string::npos) cleanName = cleanName.substr(0, nameEnd + 1);
            size_t nameStart = cleanName.find_first_not_of(" \t\r\n");
            if (nameStart != std::string::npos) cleanName = cleanName.substr(nameStart);

            if (!cleanName.empty()) {
                InvoiceItem item;
                item.description = cleanName;
                item.itemType = "Service";
                item.quantity = 1;
                item.unitPrice = (itemPrice > 0.0) ? itemPrice : inv.partsCost;
                item.totalPrice = item.unitPrice;
                parsedItems.push_back(item);
            }
        }

        if (!parsedItems.empty()) {
            for (const auto& item : parsedItems) {
                inv.items.push_back(item);
            }
        }
        else {
            InvoiceItem item;
            item.description = "General Service & Repair";
            item.itemType = "Service";
            item.quantity = 1;
            item.unitPrice = inv.partsCost;
            item.totalPrice = inv.partsCost;
            inv.items.push_back(item);
        }
    }

    // 3. Add Labor Charge row
    if (inv.laborCost > 0.0) {
        InvoiceItem laborItem;
        laborItem.description = "Labor Charge";
        laborItem.itemType = "Labor";
        laborItem.quantity = 1;
        laborItem.unitPrice = inv.laborCost;
        laborItem.totalPrice = inv.laborCost;
        inv.items.push_back(laborItem);
    }

    double subtotal = inv.partsCost + inv.laborCost;
    double discountRate = getTierDiscountRate(targetCustomer.membershipTier);
    inv.discountAmount = subtotal * discountRate;

    double taxableAmount = subtotal - inv.discountAmount;
    inv.taxAmount = taxableAmount * 0.06;
    inv.totalAmount = taxableAmount + inv.taxAmount;
    inv.isPaid = false;
    inv.paymentType = "Unpaid";

    invoices.push_back(inv);
    saveInvoicesToFile(invoices);

    std::cout << Color::BRIGHT_CYAN << "\nSUCCESS: Invoice [" << inv.invoiceId << "] generated and saved to storage!\n" << Color::RESET;
    printInvoiceDetails(inv, targetCustomer.name);
}

// Function strictly edits UNPAID invoices only
void editInvoice(std::vector<Invoice>& invoices, std::vector<Customer>& customers) {
    std::string phoneInput;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Customer Phone Number to edit bill: " << Color::RESET;
    std::cin >> phoneInput;

    std::string key = toUpper(phoneInput);
    std::vector<Invoice*> unpaidCustomerInvoices;

    for (auto& inv : invoices) {
        if (!inv.isPaid && toUpper(inv.customerId) == key) {
            unpaidCustomerInvoices.push_back(&inv);
        }
    }

    if (unpaidCustomerInvoices.empty()) {
        std::cout << Color::BRIGHT_RED << "No unpaid bills found for customer phone: [" << phoneInput << "]. Paid invoices cannot be edited.\n" << Color::RESET;
        return;
    }

    std::cout << Color::CYAN << "\n========================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "        SELECT UNPAID BILL TO EDIT      \n" << Color::RESET;
    std::cout << Color::CYAN << "========================================\n" << Color::RESET;
    for (size_t i = 0; i < unpaidCustomerInvoices.size(); ++i) {
        std::cout << Color::BRIGHT_CYAN << (i + 1) << ". " << Color::BRIGHT_WHITE << unpaidCustomerInvoices[i]->invoiceId
            << " | Plate: " << unpaidCustomerInvoices[i]->licensePlate
            << " | Total: RM" << std::fixed << std::setprecision(2) << unpaidCustomerInvoices[i]->totalAmount
            << " | Status: " << Color::BRIGHT_RED << "Unpaid\n" << Color::RESET;
    }

    int invSelect;
    std::cout << Color::BRIGHT_YELLOW << "Select bill index: " << Color::RESET;
    if (!(std::cin >> invSelect) || invSelect < 1 || invSelect > static_cast<int>(unpaidCustomerInvoices.size())) {
        std::cout << Color::BRIGHT_RED << "Invalid selection!\n" << Color::RESET;
        clearCinError();
        return;
    }

    Invoice& target = *unpaidCustomerInvoices[invSelect - 1];

    std::cout << Color::BRIGHT_CYAN << "\n1. " << Color::BRIGHT_WHITE << "Update Bill Date\n";
    std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Apply / Change Discount Rate\n";
    std::cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Delete Bill\n";
    std::cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Back\n";
    std::cout << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;
    int uc;
    if (!(std::cin >> uc) || uc == 0) { clearCinError(); return; }

    if (uc == 1) {
        std::cout << Color::BRIGHT_YELLOW << "Enter new date (YYYY-MM-DD): " << Color::RESET;
        std::cin >> target.date;
        saveInvoicesToFile(invoices);
        std::cout << Color::BRIGHT_CYAN << "Bill date updated and saved successfully.\n" << Color::RESET;
    }
    else if (uc == 2) {
        std::cout << Color::BRIGHT_YELLOW << "\nChoose Discount Option:\n" << Color::RESET;
        std::cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Apply Tier-Based Discount (Auto via Customer Membership)\n";
        std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Apply Custom Percentage Discount\n";
        std::cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Apply Custom Fixed Amount Discount (RM)\n";
        std::cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Cancel\n";
        std::cout << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;

        int dChoice;
        if (!(std::cin >> dChoice)) { std::cout << Color::BRIGHT_RED << "Invalid input!\n" << Color::RESET; clearCinError(); return; }

        double sub = target.partsCost + target.laborCost;
        double discAmount = 0.0;
        double discPercentageShown = 0.0;

        if (dChoice == 1) {
            double discRate = 0.0;
            auto custIt = std::find_if(customers.begin(), customers.end(), [&](const Customer& c) {
                return c.phone == target.customerId;
                });
            if (custIt != customers.end()) {
                discRate = getTierDiscountRate(custIt->membershipTier);
            }
            discAmount = sub * discRate;
            discPercentageShown = discRate * 100.0;
        }
        else if (dChoice == 2) {
            double pct = 0.0;
            std::cout << Color::BRIGHT_YELLOW << "Enter discount percentage (e.g., 15 for 15%): " << Color::RESET;
            std::cin >> pct;
            if (pct < 0 || pct > 100) {
                std::cout << Color::BRIGHT_RED << "Invalid percentage range!\n" << Color::RESET;
                return;
            }
            discAmount = sub * (pct / 100.0);
            discPercentageShown = pct;
        }
        else if (dChoice == 3) {
            std::cout << Color::BRIGHT_YELLOW << "Enter fixed discount amount (RM): " << Color::RESET;
            std::cin >> discAmount;
            if (discAmount < 0) discAmount = 0;
            if (discAmount > sub) discAmount = sub;
            discPercentageShown = (sub > 0) ? (discAmount / sub) * 100.0 : 0.0;
        }
        else {
            std::cout << Color::BRIGHT_YELLOW << "Discount modification cancelled.\n" << Color::RESET;
            return;
        }

        target.discountAmount = discAmount;
        target.taxAmount = (sub - target.discountAmount) * 0.06;
        target.totalAmount = (sub - target.discountAmount) + target.taxAmount;

        saveInvoicesToFile(invoices);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << Color::BRIGHT_CYAN << "Discount applied (" << discPercentageShown << "% equivalent / RM" << target.discountAmount << " off).\n";
        std::cout << "New Total (incl. 6% Tax): RM" << target.totalAmount << "\n";
        std::cout << "Changes stored to file successfully.\n" << Color::RESET;
    }
    else if (uc == 3) {
        std::cout << Color::BRIGHT_YELLOW << "Confirm Delete? (Y/N): " << Color::RESET;
        char confirm;
        std::cin >> confirm;
        if (std::toupper(confirm) == 'Y') {
            auto eraseIt = std::find_if(invoices.begin(), invoices.end(), [&](const Invoice& inv) {
                return inv.invoiceId == target.invoiceId;
                });
            if (eraseIt != invoices.end()) {
                invoices.erase(eraseIt);
            }
            saveInvoicesToFile(invoices);
            std::cout << Color::BRIGHT_CYAN << "Bill deleted and changes stored to file successfully.\n" << Color::RESET;
        }
        else {
            std::cout << Color::BRIGHT_YELLOW << "Delete cancelled.\n" << Color::RESET;
        }
    }
}

void processPayment(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments) {
    std::string phoneInput;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Customer Phone Number: " << Color::RESET;
    std::cin >> phoneInput;

    auto custIt = std::find_if(customers.begin(), customers.end(), [&](Customer& c) {
        return c.phone == phoneInput;
        });

    if (custIt == customers.end()) {
        std::cout << Color::BRIGHT_RED << "ERROR: Customer with phone [" << phoneInput << "] not found!\n" << Color::RESET;
        return;
    }

    Customer& targetCustomer = *custIt;

    std::cout << Color::CYAN << "\n========================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "               SELECT BILL              \n" << Color::RESET;
    std::cout << Color::CYAN << "========================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << "Customer: " << Color::BRIGHT_WHITE << targetCustomer.name << "\n\n" << Color::RESET;

    bool hasUnpaidBills = false;
    int counter = 1;
    for (const auto& inv : invoices) {
        if (inv.customerId == targetCustomer.phone && !inv.isPaid) {
            hasUnpaidBills = true;
            std::cout << Color::BRIGHT_CYAN << counter << ". " << Color::BRIGHT_WHITE << inv.invoiceId << " | " << inv.licensePlate
                << " | RM" << std::fixed << std::setprecision(2) << inv.totalAmount << " | " << Color::BRIGHT_RED << "Unpaid\n" << Color::RESET;
            counter++;
        }
    }

    if (!hasUnpaidBills) {
        std::cout << Color::BRIGHT_YELLOW << "INFO: No unpaid bills found for this customer.\n" << Color::RESET;
        return;
    }

    int choice;
    std::cout << Color::BRIGHT_YELLOW << "\nSelect Bill: " << Color::RESET;
    if (!(std::cin >> choice)) {
        std::cout << Color::BRIGHT_RED << "Invalid input.\n" << Color::RESET;
        clearCinError();
        return;
    }

    Invoice* selectedInvoice = nullptr;
    int curr = 1;
    for (auto& inv : invoices) {
        if (inv.customerId == targetCustomer.phone && !inv.isPaid) {
            if (curr == choice) {
                selectedInvoice = &inv;
                break;
            }
            curr++;
        }
    }

    if (!selectedInvoice) {
        std::cout << Color::BRIGHT_RED << "ERROR: Invalid selection choice!\n" << Color::RESET;
        return;
    }

    Vehicle* targetVehicle = nullptr;
    for (auto& v : targetCustomer.vehicles) {
        if (v.licensePlate == selectedInvoice->licensePlate) {
            targetVehicle = &v;
            break;
        }
    }

    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "\n--- INVOICE REVIEW PRIOR TO PAYMENT ---" << Color::RESET;
    printInvoiceDetails(*selectedInvoice, targetCustomer.name);

    std::cout << Color::CYAN << "\n=======================================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "                    PAYMENT PROCESSING                  \n" << Color::RESET;
    std::cout << Color::CYAN << "=======================================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << "Total Bill      : RM" << std::fixed << std::setprecision(2) << Color::BRIGHT_WHITE << selectedInvoice->totalAmount << "\n";

    int payChoice = 1;
    while (true) {
        std::cout << Color::BRIGHT_YELLOW << "Select Payment Method:\n" << Color::RESET;
        std::cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Cash\n";
        std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Card\n";
        std::cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "E-Wallet\n";
        std::cout << Color::BRIGHT_YELLOW << "Choice: " << Color::RESET;
        if (std::cin >> payChoice && payChoice >= 1 && payChoice <= 3) {
            break;
        }
        std::cout << Color::BRIGHT_RED << "Invalid choice! Select 1-3.\n" << Color::RESET;
        clearCinError();
    }

    double amountPaid = 0.0;
    double changeGiven = 0.0;
    std::string transactionRef;

    if (payChoice == 1) {
        selectedInvoice->paymentType = "Cash";
        while (true) {
            std::cout << Color::BRIGHT_YELLOW << "Cash Tendered (Amount Given by Customer) : RM" << Color::RESET;
            if (std::cin >> amountPaid && amountPaid >= selectedInvoice->totalAmount) {
                changeGiven = amountPaid - selectedInvoice->totalAmount;
                break;
            }
            if (amountPaid >= 0.0 && amountPaid < selectedInvoice->totalAmount) {
                std::cout << Color::BRIGHT_RED << "Insufficient cash given! Amount must be at least RM" << selectedInvoice->totalAmount << ".\n" << Color::RESET;
            }
            else {
                std::cout << Color::BRIGHT_RED << "Invalid amount. Please enter a valid number.\n" << Color::RESET;
                clearCinError();
            }
        }
    }
    else if (payChoice == 2) {
        selectedInvoice->paymentType = "Card";
        std::cout << Color::BRIGHT_YELLOW << "\n--- CREDIT CARD PAYMENT VERIFICATION ---\n" << Color::RESET;
        std::cout << Color::BRIGHT_CYAN << "Please enter 12-digit transaction reference: " << Color::RESET;

        bool validRef = false;
        int attempts = 0;
        const int MAX_ATTEMPTS = 3;

        while (!validRef && attempts < MAX_ATTEMPTS) {
            std::cin >> transactionRef;
            if (validateCardTransaction(transactionRef)) {
                validRef = true;
                std::cout << Color::BRIGHT_CYAN << "[APPROVED] Card transaction verified successfully.\n" << Color::RESET;
                std::cout << Color::BRIGHT_CYAN << "Transaction Log: Reference " << transactionRef << " processed.\n" << Color::RESET;
                amountPaid = selectedInvoice->totalAmount;
            }
            else {
                attempts++;
                if (attempts < MAX_ATTEMPTS) {
                    std::cout << Color::BRIGHT_RED << "[ERROR] Transaction reference must be 12 digits. Attempt "
                        << attempts << " of " << MAX_ATTEMPTS << ".\n" << Color::RESET;
                    std::cout << Color::BRIGHT_YELLOW << "Enter 12-digit transaction reference: " << Color::RESET;
                }
                else {
                    std::cout << Color::BRIGHT_RED << "[ERROR] Card verification failed after " << MAX_ATTEMPTS
                        << " attempts. Payment cancelled.\n" << Color::RESET;
                    return;
                }
                clearCinError();
            }
        }

        if (!validRef) {
            return;
        }
    }
    else {
        selectedInvoice->paymentType = "E-Wallet";
        amountPaid = selectedInvoice->totalAmount;
    }

    std::string payDate = getCurrentDate();
    std::cout << Color::BRIGHT_CYAN << "Payment Date    : " << Color::BRIGHT_WHITE << payDate << " (Auto-set to current date)\n" << Color::RESET;

    bool fullySettled = (amountPaid + 1e-9 >= selectedInvoice->totalAmount);

    if (fullySettled) {
        selectedInvoice->isPaid = true;
        targetCustomer.totalPurchases += selectedInvoice->totalAmount;
        updateCustomerTier(targetCustomer);

        if (targetVehicle) {
            resetVehicleServiceData(*targetVehicle);
        }

        for (auto& app : appointments) {
            if (app.licensePlate == selectedInvoice->licensePlate && app.isActive) {
                app.isActive = false;
            }
        }
    }

    saveAllData(customers, appointments, invoices);

    std::cout << Color::CYAN << "\n========================================================================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "                                OFFICIAL PAYMENT RECEIPT                                \n" << Color::RESET;
    std::cout << Color::CYAN << "========================================================================================\n" << Color::RESET;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << Color::BRIGHT_CYAN << " Bill ID        : " << Color::BRIGHT_WHITE << selectedInvoice->invoiceId << "\n";
    std::cout << Color::BRIGHT_CYAN << " Customer       : " << Color::BRIGHT_WHITE << targetCustomer.name << " (Phone: " << targetCustomer.phone << ")\n";
    std::cout << Color::BRIGHT_CYAN << " Plate          : " << Color::BRIGHT_WHITE << selectedInvoice->licensePlate << "\n";
    std::cout << Color::BRIGHT_CYAN << " Payment Date   : " << Color::BRIGHT_WHITE << payDate << "\n";
    std::cout << Color::BRIGHT_CYAN << " Method         : " << Color::BRIGHT_WHITE << selectedInvoice->paymentType << "\n";

    // Display transaction reference for card payments
    if (selectedInvoice->paymentType == "Card" && !transactionRef.empty()) {
        std::cout << Color::BRIGHT_CYAN << " Transaction Ref: " << Color::BRIGHT_WHITE << transactionRef << "\n";
    }

    std::cout << Color::CYAN << "----------------------------------------------------------------------------------------\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << std::left << std::setw(12) << " Type"
        << std::setw(34) << " Description"
        << std::setw(8) << " Qty"
        << std::setw(18) << " Unit Price(RM)"
        << " Total Price(RM)\n" << Color::RESET;
    std::cout << Color::CYAN << "----------------------------------------------------------------------------------------\n" << Color::RESET;

    if (selectedInvoice->items.empty()) {
        double defaultUnit = selectedInvoice->partsCost + selectedInvoice->laborCost;
        std::cout << Color::BRIGHT_WHITE << " " << std::left << std::setw(11) << "Service"
            << std::setw(34) << shortenText("General Service & Repair", 32)
            << std::setw(8) << 1
            << std::setw(18) << defaultUnit
            << defaultUnit << "\n" << Color::RESET;
    }
    else {
        for (const auto& item : selectedInvoice->items) {
            std::cout << Color::BRIGHT_WHITE << " " << std::left << std::setw(11) << item.itemType
                << std::setw(34) << shortenText(item.description, 32)
                << std::setw(8) << item.quantity
                << std::setw(18) << item.unitPrice
                << item.totalPrice << "\n" << Color::RESET;
        }
    }

    double subtotal = selectedInvoice->partsCost + selectedInvoice->laborCost;
    std::cout << Color::CYAN << "----------------------------------------------------------------------------------------\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << " Subtotal (Parts: RM" << selectedInvoice->partsCost << " | Labor: RM" << selectedInvoice->laborCost << "): RM" << subtotal << "\n";
    std::cout << Color::BRIGHT_CYAN << " Discount       : -RM" << selectedInvoice->discountAmount << "\n";
    std::cout << Color::BRIGHT_CYAN << " Service Tax 6% : RM" << selectedInvoice->taxAmount << "\n";
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << " Total Bill     : RM" << selectedInvoice->totalAmount << "\n" << Color::RESET;
    if (selectedInvoice->paymentType == "Cash") {
        std::cout << Color::BRIGHT_CYAN << " Cash Given     : RM" << amountPaid << "\n";
        std::cout << Color::BRIGHT_CYAN << " Change Due     : RM" << changeGiven << "\n";
    }
    else {
        std::cout << Color::BRIGHT_CYAN << " Amount Paid    : RM" << amountPaid << "\n";
    }
    std::cout << Color::BRIGHT_CYAN << " Status         : " << (fullySettled ? (Color::BRIGHT_CYAN + "PAID IN FULL" + Color::RESET) : (Color::BRIGHT_RED + "PARTIAL - Balance remaining" + Color::RESET)) << "\n";
    std::cout << Color::CYAN << "========================================================================================\n" << Color::RESET;
}

void paymentHistory(const std::vector<Customer>& customers, const std::vector<Invoice>& invoices) {
    int choice = -1;
    do {
        clearScreen();
        std::cout << Color::CYAN << "\n========================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "              PAYMENT HISTORY            \n" << Color::RESET;
        std::cout << Color::CYAN << "========================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "View Unpaid Bills\n";
        std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Search Invoice (Paid & Unpaid) by Phone Number\n";
        std::cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "View Customer Billing History\n";
        std::cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Back\n";
        std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;
        if (!(std::cin >> choice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input!\n" << Color::RESET;
            clearCinError();
            pauseSystem();
            continue;
        }

        if (choice == 1) {
            clearScreen();
            displayInvoices(invoices, customers);
            pauseSystem();
        }
        else if (choice == 2) {
            clearScreen();
            searchInvoiceByPhone(const_cast<std::vector<Invoice>&>(invoices), const_cast<std::vector<Customer>&>(customers));
            pauseSystem();
        }
        else if (choice == 3) {
            clearScreen();
            viewCustomerHistory(customers, invoices);
            pauseSystem();
        }
        else if (choice != 0) {
            std::cout << Color::BRIGHT_RED << "Invalid choice! Please select an option between 0 and 3.\n" << Color::RESET;
            pauseSystem();
        }
    } while (choice != 0);
}

void viewCustomerHistory(const std::vector<Customer>& customers, const std::vector<Invoice>& invoices) {
    std::string input;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Customer Phone Number (or 0 to Return): " << Color::RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, input);
    if (input == "0") return;

    std::string searchKey = toUpper(input);
    const Customer* foundCustomer = nullptr;

    for (const auto& c : customers) {
        if (c.phone == searchKey) {
            foundCustomer = &c;
            break;
        }
    }

    if (!foundCustomer) {
        std::cout << Color::BRIGHT_RED << "ERROR: No customer record found for Phone Number [" << searchKey << "]!\n" << Color::RESET;
        return;
    }

    std::cout << Color::CYAN << "\n=======================================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "                CUSTOMER BILLING HISTORY               \n" << Color::RESET;
    std::cout << Color::CYAN << "=======================================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << "Name           : " << Color::BRIGHT_WHITE << foundCustomer->name << "\n";
    std::cout << Color::BRIGHT_CYAN << "Phone          : " << Color::BRIGHT_WHITE << foundCustomer->phone << "\n";
    std::cout << Color::BRIGHT_CYAN << "Membership Tier: " << Color::BRIGHT_WHITE << foundCustomer->membershipTier << "\n";
    std::cout << Color::BRIGHT_CYAN << "Total Spending : RM" << std::fixed << std::setprecision(2) << Color::BRIGHT_WHITE << foundCustomer->totalPurchases << "\n";
    std::cout << Color::CYAN << "-------------------------------------------------------\n" << Color::RESET;

    std::cout << Color::BRIGHT_YELLOW << "TRANSACTION & BILL HISTORY:\n" << Color::RESET;
    bool hasInvoices = false;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << std::left << std::setw(10) << "Bill ID"
        << std::setw(12) << "Plate"
        << std::setw(12) << "Date"
        << std::setw(12) << "Total(RM)"
        << std::setw(12) << "Method"
        << "Status\n" << Color::RESET;
    std::cout << Color::CYAN << "-------------------------------------------------------\n" << Color::RESET;

    for (const auto& inv : invoices) {
        if (inv.customerId == foundCustomer->phone) {
            hasInvoices = true;
            std::cout << Color::BRIGHT_WHITE << std::left << std::setw(10) << inv.invoiceId
                << std::setw(12) << inv.licensePlate
                << std::setw(12) << inv.date
                << std::setw(12) << std::fixed << std::setprecision(2) << inv.totalAmount
                << std::setw(12) << inv.paymentType
                << (inv.isPaid ? (Color::BRIGHT_CYAN + "Paid" + Color::RESET) : (Color::BRIGHT_RED + "Unpaid" + Color::RESET)) << "\n";
        }
    }

    if (!hasInvoices) {
        std::cout << Color::BRIGHT_YELLOW << "No past bill records found for this customer.\n" << Color::RESET;
    }
    std::cout << Color::CYAN << "=======================================================\n" << Color::RESET;
}