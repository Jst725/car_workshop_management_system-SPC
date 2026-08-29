#include "service.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>
#include <vector>
#include <cstdlib>
#include <limits>

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

static void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void clearCinError() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static void pauseConsole() {
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

static std::string shortenText(const std::string& str, size_t maxLen = 24) {
    if (str.length() <= maxLen) return str;
    return str.substr(0, maxLen - 3) + "...";
}

static std::string getTodayDate() {
    time_t now = time(nullptr);
    tm local;
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    local = *localtime(&now);
#endif
    std::stringstream ss;
    ss << (1900 + local.tm_year) << "-"
        << std::setw(2) << std::setfill('0') << (local.tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << local.tm_mday;
    return ss.str();
}

static bool hasActiveAppointment(const std::string& plate, const std::vector<Appointment>& appointments) {
    std::string searchPlate = toUpper(plate);
    for (const auto& a : appointments) {
        if (a.isActive && toUpper(a.licensePlate) == searchPlate) {
            return true;
        }
    }
    return false;
}

static std::vector<std::pair<std::string, double>> parseServiceItems(const std::string& input) {
    std::vector<std::pair<std::string, double>> items;

    std::stringstream ss(input);
    std::string segment;

    while (std::getline(ss, segment, '|')) {
        double price = 0.0;

        // 1. Extract price enclosed in [$...] or [RM...] before stripping brackets
        std::smatch priceMatch;
        std::regex priceRegex(R"(\[\s*[\$|RM]*\s*([\d\.]+)\s*\])");
        if (std::regex_search(segment, priceMatch, priceRegex)) {
            price = std::stod(priceMatch[1].str());
            segment = std::regex_replace(segment, priceRegex, "");
        }

        // 2. Remove category headers like [Scheduled Maintenance / Servicing]
        std::string cleaned = std::regex_replace(segment, std::regex(R"(\[.*?\])"), "");

        // 3. Extract description after '->' if present
        size_t arrowPos = cleaned.find("->");
        if (arrowPos != std::string::npos) {
            cleaned = cleaned.substr(arrowPos + 2);
        }

        // 4. Clean sequence numbers like (1), (2) and summary trailers
        cleaned = std::regex_replace(cleaned, std::regex(R"(\(\d+\))"), "");
        cleaned = std::regex_replace(cleaned, std::regex(R"(\{.*?\})"), "");
        cleaned = std::regex_replace(cleaned, std::regex(R"(\(RM\s*[\d\.]+\))"), "");

        // 5. Trim whitespace
        size_t start = cleaned.find_first_not_of(" \t\n\r");
        size_t end = cleaned.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos) {
            cleaned = cleaned.substr(start, end - start + 1);
            items.push_back({ cleaned, price });
        }
    }

    return items;
}

static void syncAppointmentToPartsCost(Vehicle& vehicle, const std::vector<Appointment>& appointments) {
    std::string today = getTodayDate();

    for (const auto& app : appointments) {
        if (app.isActive && toUpper(app.licensePlate) == toUpper(vehicle.licensePlate)) {

            // Parse multi-item string into short line items
            auto parsedItems = parseServiceItems(app.serviceType);

            for (size_t idx = 0; idx < parsedItems.size(); ++idx) {
                std::string itemPartId = "APT-ITEM-" + app.appointmentId + "-" + std::to_string(idx);

                bool itemExists = false;
                for (const auto& p : vehicle.parts) {
                    if (p.partId == itemPartId) {
                        itemExists = true;
                        break;
                    }
                }

                if (!itemExists) {
                    PartItem part;
                    part.partId = itemPartId;
                    part.name = parsedItems[idx].first;
                    part.quantity = 1;
                    part.unitPrice = parsedItems[idx].second;
                    part.price = part.unitPrice;
                    part.isFromAppointment = true;
                    vehicle.parts.push_back(part);
                }
            }
        }
    }
    vehicle.partsCost = 0.0;
    vehicle.activeServiceLogs.clear();
    for (const auto& item : vehicle.parts) {
        vehicle.partsCost += item.price;

        ServiceLog log;
        log.date = today;
        log.description = item.name;
        log.cost = item.price;
        vehicle.activeServiceLogs.push_back(log);
    }

    // Accrued total formula
    vehicle.currentServiceCost = vehicle.partsCost + vehicle.laborCost;
}

void searchVehicleHistory(const std::vector<Customer>& customers);

void vehicleMenu(std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    int choice = -1;
    do {
        clearScreen();
        std::cout << Color::CYAN << "\n========================================\n" << Color::RESET
            << Color::BRIGHT_YELLOW << Color::BOLD << "          VEHICLE MANAGEMENT            \n" << Color::RESET
            << Color::CYAN << "========================================\n" << Color::RESET
            << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Update Vehicle Status\n"
            << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Update Service & Pricing\n"
            << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "View Itemized Service Breakdown\n"
            << Color::BRIGHT_CYAN << "4. " << Color::BRIGHT_WHITE << "View Workshop Vehicles Dashboard\n"
            << Color::BRIGHT_CYAN << "5. " << Color::BRIGHT_WHITE << "Check Service Reminders\n"
            << Color::BRIGHT_CYAN << "6. " << Color::BRIGHT_WHITE << "Search Vehicle Database\n"
            << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Return to Main Menu\n"
            << Color::CYAN << "----------------------------------------\n" << Color::RESET
            << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;

        if (!(std::cin >> choice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
            clearCinError();
            pauseConsole();
            continue;
        }

        switch (choice) {
        case 1:
            clearScreen();
            updateVehicleStatus(customers, appointments);
            saveCustomersToFile(customers);
            pauseConsole();
            break;
        case 2:
            clearScreen();
            updateService(customers, appointments);
            saveCustomersToFile(customers);
            pauseConsole();
            break;
        case 3:
            clearScreen();
            viewServiceBreakdown(customers, appointments);
            pauseConsole();
            break;
        case 4:
            clearScreen();
            displayWorkshopVehicle(customers, appointments);
            pauseConsole();
            break;
        case 5:
            clearScreen();
            checkServiceReminders(customers, appointments);
            pauseConsole();
            break;
        case 6:
            clearScreen();
            searchVehicleHistory(customers);
            pauseConsole();
            break;
        case 0:
            std::cout << Color::BRIGHT_CYAN << "Returning to Main Menu...\n" << Color::RESET;
            break;
        default:
            std::cout << Color::BRIGHT_RED << "Invalid choice! Please select an option between 0 and 6.\n" << Color::RESET;
            pauseConsole();
        }
    } while (choice != 0);
}

void updateVehicleStatus(std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::string plate;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Vehicle License Plate (or 0 to Return): " << Color::RESET;
    std::cin >> plate;
    if (plate == "0") return;

    plate = toUpper(plate);

    if (!hasActiveAppointment(plate, appointments)) {
        std::cout << Color::BRIGHT_RED << "ERROR: Vehicle [" << plate << "] has no active appointment booking!\n" << Color::RESET;
        return;
    }

    for (auto& c : customers) {
        for (auto& v : c.vehicles) {
            if (toUpper(v.licensePlate) == plate) {
                int st = -1;
                while (true) {
                    std::cout << Color::CYAN << "\nSelect Current Status:\n" << Color::RESET
                        << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Awaiting Parts\n"
                        << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "In Progress\n"
                        << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Ready for Pickup\n"
                        << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Return\n"
                        << Color::BRIGHT_YELLOW << "Choice: " << Color::RESET;
                    if (std::cin >> st && st >= 0 && st <= 3) break;
                    std::cout << Color::BRIGHT_RED << "Invalid choice! Enter 0-3.\n" << Color::RESET;
                    clearCinError();
                }

                if (st == 0) return;
                switch (st) {
                case 1: v.status = "Awaiting Parts"; break;
                case 2: v.status = "In Progress"; break;
                case 3:
                    v.status = "Ready for Pickup";
                    v.lastServiceDate = getTodayDate();
                    if (v.currentServiceCost == 0.0) {
                        syncAppointmentToPartsCost(v, appointments);
                    }
                    break;
                }

                std::cout << Color::BRIGHT_YELLOW << "Assign Technician Name: " << Color::RESET;
                std::cin.ignore(1000, '\n');
                std::getline(std::cin, v.assignedTechnician);

                std::cout << Color::BRIGHT_CYAN << "Status successfully updated for " << Color::BRIGHT_WHITE << v.licensePlate << "!\n" << Color::RESET;
                return;
            }
        }
    }
    std::cout << Color::BRIGHT_RED << "Vehicle plate not found in database.\n" << Color::RESET;
}

void updateService(std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::string plate;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Vehicle License Plate (or 0 to Return): " << Color::RESET;
    std::cin >> plate;
    if (plate == "0") return;

    plate = toUpper(plate);

    if (!hasActiveAppointment(plate, appointments)) {
        std::cout << Color::BRIGHT_RED << "ERROR: Vehicle [" << plate << "] has no active appointment booking!\n" << Color::RESET;
        return;
    }

    for (auto& c : customers) {
        for (auto& v : c.vehicles) {
            if (toUpper(v.licensePlate) == plate) {

                syncAppointmentToPartsCost(v, appointments);

                int choice = -1;
                do {
                    clearScreen();
                    std::cout << Color::CYAN << "\n==========================================================================\n" << Color::RESET
                        << Color::BRIGHT_YELLOW << Color::BOLD << "         EDIT SERVICE & PRICING: " << Color::BRIGHT_WHITE << v.licensePlate << "\n" << Color::RESET
                        << Color::CYAN << "==========================================================================\n" << Color::RESET
                        << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Add Additional Part / Component\n"
                        << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Labor Charge (Current: RM " << std::fixed << std::setprecision(2) << v.laborCost << ")\n"
                        << Color::CYAN << "--------------------------------------------------------------------------\n" << Color::RESET
                        << Color::BRIGHT_YELLOW << "Logged Appointment Services & Workshop Parts:\n" << Color::RESET;

                    int optionNum = 3;
                    if (v.parts.empty()) {
                        std::cout << Color::LIGHT_GRAY << "   (No parts or appointment services logged)\n" << Color::RESET;
                    }
                    else {
                        std::cout << Color::BRIGHT_CYAN << std::left
                            << std::setw(5) << "No."
                            << std::setw(26) << "Description"
                            << std::setw(10) << "Type"
                            << std::setw(6) << "Qty"
                            << std::setw(12) << "Unit(RM)"
                            << "Total(RM)\n" << Color::RESET
                            << Color::CYAN << "--------------------------------------------------------------------------\n" << Color::RESET;
                        for (size_t i = 0; i < v.parts.size(); ++i) {
                            const PartItem& p = v.parts[i];
                            std::cout << Color::BRIGHT_WHITE << std::left
                                << std::setw(5) << optionNum++
                                << std::setw(26) << shortenText(p.name)
                                << std::setw(10) << (p.isFromAppointment ? "Locked" : "Workshop")
                                << std::setw(6) << p.quantity
                                << std::setw(12) << std::fixed << std::setprecision(2) << p.unitPrice
                                << std::fixed << std::setprecision(2) << p.price << "\n" << Color::RESET;
                        }
                    }
                    std::cout << Color::CYAN << "--------------------------------------------------------------------------\n" << Color::RESET
                        << Color::BRIGHT_CYAN << "Parts Subtotal: RM " << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << v.partsCost
                        << Color::BRIGHT_CYAN << "   |   Labor: RM " << Color::BRIGHT_WHITE << v.laborCost
                        << Color::BRIGHT_CYAN << "   |   TOTAL: RM " << Color::BRIGHT_WHITE << v.currentServiceCost << "\n"
                        << Color::CYAN << "--------------------------------------------------------------------------\n" << Color::RESET
                        << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Save & Exit\n"
                        << Color::CYAN << "--------------------------------------------------------------------------\n" << Color::RESET
                        << Color::BRIGHT_YELLOW << "Select item number to change (0 to finish): " << Color::RESET;

                    if (!(std::cin >> choice)) {
                        std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
                        clearCinError();
                        pauseConsole();
                        continue;
                    }

                    if (choice == 0) {
                        break;
                    }
                    else if (choice == 1) {
                        PartItem p;
                        int qty = 1;
                        double unitPrice = 0.0;

                        std::cout << Color::BRIGHT_YELLOW << "Enter Part Name: " << Color::RESET;
                        std::cin.ignore(1000, '\n');
                        std::getline(std::cin, p.name);

                        while (true) {
                            std::cout << Color::BRIGHT_YELLOW << "Enter Quantity: " << Color::RESET;
                            if (std::cin >> qty && qty > 0) break;
                            std::cout << Color::BRIGHT_RED << "Invalid quantity!\n" << Color::RESET;
                            clearCinError();
                        }

                        while (true) {
                            std::cout << Color::BRIGHT_YELLOW << "Enter Unit Price for " << p.name << ": RM " << Color::RESET;
                            if (std::cin >> unitPrice && unitPrice >= 0.0) break;
                            std::cout << Color::BRIGHT_RED << "Invalid price!\n" << Color::RESET;
                            clearCinError();
                        }

                        p.quantity = qty;
                        p.unitPrice = unitPrice;
                        p.price = qty * unitPrice;
                        p.isFromAppointment = false;
                        v.parts.push_back(p);
                    }
                    else if (choice == 2) {
                        double newLabor = 0.0;
                        while (true) {
                            std::cout << Color::BRIGHT_YELLOW << "Enter Labor Charge: RM " << Color::RESET;
                            if (std::cin >> newLabor && newLabor >= 0.0) break;
                            std::cout << Color::BRIGHT_RED << "Invalid labor charge!\n" << Color::RESET;
                            clearCinError();
                        }
                        v.laborCost = newLabor;
                    }
                    else if (choice >= 3 && choice < 3 + static_cast<int>(v.parts.size())) {
                        int partIdx = choice - 3;

                        if (v.parts[partIdx].isFromAppointment) {
                            std::cout << Color::BRIGHT_RED << "\n[BLOCKED] \"" << v.parts[partIdx].name << "\" is the service/problem "
                                "the customer booked at their appointment. Workshop staff cannot change its cost "
                                "or description here - only the Appointment module can modify what was booked.\n" << Color::RESET;
                            pauseConsole();
                            continue;
                        }

                        PartItem& target = v.parts[partIdx];
                        std::cout << Color::CYAN << "\n--- EDIT ITEM: " << Color::BRIGHT_WHITE << target.name << Color::CYAN << " (Qty " << target.quantity
                            << " x RM " << std::fixed << std::setprecision(2) << target.unitPrice << ") ---\n" << Color::RESET
                            << Color::BRIGHT_CYAN << "1. Edit Quantity\n"
                            << Color::BRIGHT_CYAN << "2. Edit Unit Price\n"
                            << Color::BRIGHT_CYAN << "3. Edit Description\n"
                            << Color::BRIGHT_RED << "4. Remove Item\n"
                            << Color::BRIGHT_YELLOW << "Choice: " << Color::RESET;

                        int subChoice = 0;
                        if (std::cin >> subChoice) {
                            if (subChoice == 1) {
                                int newQty = 0;
                                std::cout << Color::BRIGHT_YELLOW << "Enter New Quantity: " << Color::RESET;
                                if (std::cin >> newQty && newQty > 0) {
                                    target.quantity = newQty;
                                    target.price = target.quantity * target.unitPrice;
                                }
                                else {
                                    std::cout << Color::BRIGHT_RED << "Invalid quantity - no change made.\n" << Color::RESET;
                                    clearCinError();
                                }
                            }
                            else if (subChoice == 2) {
                                double newUnitPrice = 0.0;
                                std::cout << Color::BRIGHT_YELLOW << "Enter New Unit Price: RM " << Color::RESET;
                                if (std::cin >> newUnitPrice && newUnitPrice >= 0.0) {
                                    target.unitPrice = newUnitPrice;
                                    target.price = target.quantity * target.unitPrice;
                                }
                                else {
                                    std::cout << Color::BRIGHT_RED << "Invalid price - no change made.\n" << Color::RESET;
                                    clearCinError();
                                }
                            }
                            else if (subChoice == 3) {
                                std::cout << Color::BRIGHT_YELLOW << "Enter New Name: " << Color::RESET;
                                std::cin.ignore(1000, '\n');
                                std::getline(std::cin, target.name);
                            }
                            else if (subChoice == 4) {
                                v.parts.erase(v.parts.begin() + partIdx);
                            }
                        }
                    }

                    v.partsCost = 0.0;
                    v.activeServiceLogs.clear();
                    for (const auto& item : v.parts) {
                        v.partsCost += item.price;
                        ServiceLog logEntry;
                        logEntry.date = getTodayDate();
                        logEntry.description = item.name;
                        logEntry.cost = item.price;
                        v.activeServiceLogs.push_back(logEntry);
                    }
                    v.currentServiceCost = v.partsCost + v.laborCost;
                    v.lastServiceDate = getTodayDate();

                } while (choice != 0);

                std::cout << Color::CYAN << "\n==================================================\n" << Color::RESET
                    << Color::BRIGHT_YELLOW << Color::BOLD << "   FINAL UPDATED TOTALS FOR " << v.licensePlate << "\n" << Color::RESET
                    << Color::CYAN << "==================================================\n" << Color::RESET
                    << Color::BRIGHT_CYAN << "Parts Subtotal : RM " << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << v.partsCost << "\n"
                    << Color::BRIGHT_CYAN << "Labor Subtotal : RM " << Color::BRIGHT_WHITE << v.laborCost << "\n"
                    << Color::BRIGHT_CYAN << "TOTAL COST     : RM " << Color::BRIGHT_WHITE << v.currentServiceCost << "\n"
                    << Color::CYAN << "==================================================\n" << Color::RESET;
                return;
            }
        }
    }
    std::cout << Color::BRIGHT_RED << "Vehicle plate not found in database.\n" << Color::RESET;
}

void viewServiceBreakdown(const std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::string plate;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Vehicle License Plate (or 0 to Return): " << Color::RESET;
    std::cin >> plate;
    if (plate == "0") return;

    plate = toUpper(plate);

    if (!hasActiveAppointment(plate, appointments)) {
        std::cout << Color::BRIGHT_RED << "ERROR: Vehicle [" << plate << "] has no active appointment booking!\n" << Color::RESET;
        return;
    }

    // Cast away constness on customer loop to safely synchronize vehicle appointments if parts vector is missing entries
    for (auto& c : const_cast<std::vector<Customer>&>(customers)) {
        for (auto& v : c.vehicles) {
            if (toUpper(v.licensePlate) == plate) {

                // Ensure active appointment services are populated before displaying breakdown
                syncAppointmentToPartsCost(v, appointments);

                clearScreen();
                std::cout << Color::CYAN << "\n==========================================================================\n" << Color::RESET
                    << Color::BRIGHT_YELLOW << Color::BOLD << "             ITEMIZED SERVICE BREAKDOWN: " << Color::BRIGHT_WHITE << v.licensePlate << "\n" << Color::RESET
                    << Color::CYAN << "==========================================================================\n" << Color::RESET
                    << Color::BRIGHT_CYAN << "Vehicle Model : " << Color::BRIGHT_WHITE << v.brand << " " << v.model << "\n"
                    << Color::BRIGHT_CYAN << "Owner Name    : " << Color::BRIGHT_WHITE << c.name << " (Ph: " << c.phone << ")\n"
                    << Color::BRIGHT_CYAN << "Technician    : " << Color::BRIGHT_WHITE << (v.assignedTechnician.empty() ? "Unassigned" : v.assignedTechnician) << "\n"
                    << Color::BRIGHT_CYAN << "Current Status: " << Color::BRIGHT_WHITE << v.status << "\n"
                    << Color::CYAN << "--------------------------------------------------------------------------\n" << Color::RESET
                    << Color::BRIGHT_YELLOW << "Logged Appointment Services & Workshop Parts:\n" << Color::RESET;

                if (v.parts.empty()) {
                    std::cout << Color::LIGHT_GRAY << "   (No parts or appointment services logged)\n" << Color::RESET;
                }
                else {
                    std::cout << Color::BRIGHT_CYAN << std::left
                        << std::setw(5) << "No."
                        << std::setw(32) << "Description"
                        << std::setw(6) << "Qty"
                        << std::setw(12) << "Unit(RM)"
                        << "Total(RM)\n" << Color::RESET
                        << Color::CYAN << "--------------------------------------------------------------------------\n" << Color::RESET;

                    for (size_t i = 0; i < v.parts.size(); ++i) {
                        const PartItem& p = v.parts[i];
                        std::cout << Color::BRIGHT_WHITE << std::left
                            << std::setw(5) << (i + 1)
                            << std::setw(32) << shortenText(p.name, 30)
                            << std::setw(6) << p.quantity
                            << std::setw(12) << std::fixed << std::setprecision(2) << p.unitPrice
                            << std::fixed << std::setprecision(2) << p.price << "\n" << Color::RESET;
                    }
                }

                std::cout << Color::CYAN << "--------------------------------------------------------------------------\n" << Color::RESET
                    << Color::BRIGHT_CYAN << "Parts Subtotal: RM " << Color::BRIGHT_WHITE << std::fixed << std::setprecision(2) << v.partsCost
                    << Color::BRIGHT_CYAN << "   |   Labor: RM " << Color::BRIGHT_WHITE << v.laborCost
                    << Color::BRIGHT_CYAN << "   |   TOTAL: RM " << Color::BRIGHT_WHITE << v.currentServiceCost << "\n"
                    << Color::CYAN << "==========================================================================\n" << Color::RESET;

                return;
            }
        }
    }
    std::cout << Color::BRIGHT_RED << "Vehicle plate not found in database.\n" << Color::RESET;
}

void displayWorkshopVehicle(const std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::cout << Color::CYAN << "\n--- DASHBOARD VIEW OPTIONS ---\n" << Color::RESET
        << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "View All Vehicles with Active Appointments\n"
        << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Filter by Status (Awaiting Parts / In Progress / Ready)\n"
        << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Filter by Technician\n"
        << Color::BRIGHT_YELLOW << "Choice: " << Color::RESET;

    int filterChoice = 1;
    if (!(std::cin >> filterChoice)) {
        clearCinError();
        filterChoice = 1;
    }

    std::string targetStatus = "";
    std::string targetTech = "";

    if (filterChoice == 2) {
        std::cout << Color::BRIGHT_YELLOW << "1. Awaiting Parts | 2. In Progress | 3. Ready for Pickup: " << Color::RESET;
        int sChoice;
        std::cin >> sChoice;
        if (sChoice == 1) targetStatus = "Awaiting Parts";
        else if (sChoice == 2) targetStatus = "In Progress";
        else if (sChoice == 3) targetStatus = "Ready for Pickup";
    }
    else if (filterChoice == 3) {
        std::cout << Color::BRIGHT_YELLOW << "Enter Technician Name to filter: " << Color::RESET;
        std::cin.ignore(1000, '\n');
        std::getline(std::cin, targetTech);
    }

    clearScreen();
    std::cout << Color::CYAN << "\n===================================================================================================================\n" << Color::RESET
        << Color::BRIGHT_CYAN << std::left << std::setw(12) << "Plate"
        << std::setw(16) << "Vehicle"
        << std::setw(18) << "Status"
        << std::setw(18) << "Technician"
        << std::setw(18) << "Owner"
        << "Accrued Cost (RM)\n"
        << Color::CYAN << "------------------------------------------------------------------------------------------------------------------\n" << Color::RESET;

    bool empty = true;
    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (!hasActiveAppointment(v.licensePlate, appointments)) {
                continue;
            }

            if (!targetStatus.empty() && v.status != targetStatus) continue;
            if (!targetTech.empty() && toUpper(v.assignedTechnician).find(toUpper(targetTech)) == std::string::npos) continue;

            empty = false;
            std::cout << Color::BRIGHT_WHITE << std::left << std::setw(12) << v.licensePlate
                << std::setw(16) << (v.brand + " " + v.model)
                << std::setw(18) << v.status
                << std::setw(18) << v.assignedTechnician
                << std::setw(18) << c.name
                << std::fixed << std::setprecision(2) << v.currentServiceCost << "\n" << Color::RESET;
        }
    }
    if (empty) {
        std::cout << Color::BRIGHT_YELLOW << "No active appointment vehicles currently found in workshop.\n" << Color::RESET;
    }
}

void checkServiceReminders(const std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::cout << Color::CYAN << "\n=====================================================================================\n" << Color::RESET
        << Color::BRIGHT_YELLOW << Color::BOLD << "                       AUTOMATED SERVICE REMINDER DASHBOARD           \n" << Color::RESET
        << Color::CYAN << "=====================================================================================\n" << Color::RESET;

    std::string today = getTodayDate();
    bool overdueFound = false;

    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (!hasActiveAppointment(v.licensePlate, appointments)) {
                continue;
            }

            if (v.lastServiceDate == "N/A" || v.lastServiceDate < today) {
                std::cout << Color::BRIGHT_RED << "[OVERDUE FLAG] " << Color::BRIGHT_WHITE << c.name << " (" << c.phone << ")"
                    << " | Car: " << v.licensePlate << " (" << v.brand << ")"
                    << " | Last Serviced: " << v.lastServiceDate << "\n" << Color::RESET;
                overdueFound = true;
            }
        }
    }
    if (!overdueFound) std::cout << Color::BRIGHT_CYAN << "All active appointment vehicles are up to date on service schedules!\n" << Color::RESET;
}

void searchVehicleHistory(const std::vector<Customer>& customers) {
    std::string keyword;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter keyword to search (Plate, Brand, or Model): " << Color::RESET;
    std::cin.ignore(1000, '\n');
    std::getline(std::cin, keyword);
    keyword = toUpper(keyword);

    clearScreen();
    std::cout << Color::CYAN << "\n======================================================\n" << Color::RESET
        << Color::BRIGHT_YELLOW << Color::BOLD << "            VEHICLE SEARCH RESULTS         \n" << Color::RESET
        << Color::CYAN << "======================================================\n" << Color::RESET;

    bool found = false;
    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (toUpper(v.licensePlate).find(keyword) != std::string::npos ||
                toUpper(v.brand).find(keyword) != std::string::npos ||
                toUpper(v.model).find(keyword) != std::string::npos) {
                found = true;
                std::cout << Color::BRIGHT_CYAN << "Plate         : " << Color::BRIGHT_WHITE << v.licensePlate << "\n"
                    << Color::BRIGHT_CYAN << "Make/Model    : " << Color::BRIGHT_WHITE << v.brand << " " << v.model << "\n"
                    << Color::BRIGHT_CYAN << "Owner         : " << Color::BRIGHT_WHITE << c.name << " (Ph: " << c.phone << ")\n"
                    << Color::BRIGHT_CYAN << "Current Status: " << Color::BRIGHT_WHITE << v.status << "\n"
                    << Color::BRIGHT_CYAN << "Last Serviced : " << Color::BRIGHT_WHITE << v.lastServiceDate << "\n"
                    << Color::CYAN << "------------------------------------------------------\n" << Color::RESET;
            }
        }
    }
    if (!found) {
        std::cout << Color::BRIGHT_YELLOW << "No matching vehicles found for keyword: " << keyword << "\n" << Color::RESET;
    }
}

void resetVehicleServiceData(Vehicle& vehicle) {
    vehicle.parts.clear();
    vehicle.activeServiceLogs.clear();
    vehicle.partsCost = 0.0;
    vehicle.laborCost = 0.0;
    vehicle.currentServiceCost = 0.0;
    vehicle.status = "Completed";
}