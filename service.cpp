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

static std::string toUpper(std::string str) {
    for (char& c : str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return str;
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

// Parses raw appointment strings into individual short service names and prices
static std::vector<std::pair<std::string, double>> parseServiceItems(const std::string& input) {
    std::vector<std::pair<std::string, double>> items;

    // Splits multi-item string by '|' delimiter
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

    // Explicitly reset labor cost to 0.00
    vehicle.laborCost = 0.0;

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
                    part.name = parsedItems[idx].first;   // Short description
                    part.price = parsedItems[idx].second; // Fee or RM 0.00 if unpriced
                    vehicle.parts.push_back(part);
                }
            }
        }
    }

    // Recalculate parts subtotal and update active service logs
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
void updateAllVehicleStatuses(std::vector<Customer>& customers, const std::vector<Appointment>& appointments);

void vehicleMenu(std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "          VEHICLE MANAGEMENT            \n";
        std::cout << "========================================\n";
        std::cout << "1. Update Vehicle Status\n";
        std::cout << "2. Update Service & Pricing\n";
        std::cout << "3. View Itemized Service Breakdown\n";
        std::cout << "4. View Workshop Vehicles Dashboard\n";
        std::cout << "5. Check Service Reminders\n";
        std::cout << "6. Search Vehicle Database\n";
        std::cout << "7. Bulk Update Workshop Statuses\n";
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
        case 1:
            updateVehicleStatus(customers, appointments);
            saveCustomersToFile(customers);
            break;
        case 2:
            updateService(customers, appointments);
            saveCustomersToFile(customers);
            break;
        case 3:
            viewServiceBreakdown(customers, appointments);
            break;
        case 4:
            displayWorkshopVehicle(customers, appointments);
            break;
        case 5:
            checkServiceReminders(customers, appointments);
            break;
        case 6:
            searchVehicleHistory(customers);
            break;
        case 7:
            updateAllVehicleStatuses(customers, appointments);
            saveCustomersToFile(customers);
            break;
        case 0:
            std::cout << "Returning to Main Menu...\n";
            break;
        default:
            std::cout << "Invalid choice! Please select an option between 0 and 7.\n";
        }
    } while (choice != 0);
}

void updateVehicleStatus(std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::string plate;
    std::cout << "\nEnter Vehicle License Plate (or 0 to Return): ";
    std::cin >> plate;
    if (plate == "0") return;

    plate = toUpper(plate);

    if (!hasActiveAppointment(plate, appointments)) {
        std::cout << "ERROR: Vehicle [" << plate << "] has no active appointment booking!\n";
        return;
    }

    for (auto& c : customers) {
        for (auto& v : c.vehicles) {
            if (toUpper(v.licensePlate) == plate) {
                int st = -1;
                while (true) {
                    std::cout << "\nSelect Current Status:\n";
                    std::cout << "1. Awaiting Parts\n";
                    std::cout << "2. In Progress\n";
                    std::cout << "3. Ready for Pickup\n";
                    std::cout << "0. Return\n";
                    std::cout << "Choice: ";
                    if (std::cin >> st && st >= 0 && st <= 3) break;
                    std::cout << "Invalid choice! Enter 0-3.\n";
                    std::cin.clear();
                    std::cin.ignore(1000, '\n');
                }

                if (st == 0) return;
                switch (st) {
                case 1: v.status = "Awaiting Parts"; break;
                case 2: v.status = "In Progress"; break;
                case 3:
                    v.status = "Ready for Pickup";
                    v.lastServiceDate = getTodayDate();
                    // Ensure costs are synced so invoice generator can process it
                    if (v.currentServiceCost == 0.0) {
                        syncAppointmentToPartsCost(v, appointments);
                    }
                    break;
                }

                std::cout << "Assign Technician Name: ";
                std::cin.ignore(1000, '\n');
                std::getline(std::cin, v.assignedTechnician);

                std::cout << "Status successfully updated for " << v.licensePlate << "!\n";
                return;
            }
        }
    }
    std::cout << "Vehicle plate not found in database.\n";
}

void updateService(std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::string plate;
    std::cout << "\nEnter Vehicle License Plate (or 0 to Return): ";
    std::cin >> plate;
    if (plate == "0") return;

    plate = toUpper(plate);

    if (!hasActiveAppointment(plate, appointments)) {
        std::cout << "ERROR: Vehicle [" << plate << "] has no active appointment booking!\n";
        return;
    }

    for (auto& c : customers) {
        for (auto& v : c.vehicles) {
            if (toUpper(v.licensePlate) == plate) {

                syncAppointmentToPartsCost(v, appointments);

                int choice = -1;
                do {
                    std::cout << "\n========================================\n";
                    std::cout << "   EDIT SERVICE & PRICING: " << v.licensePlate << "\n";
                    std::cout << "========================================\n";
                    std::cout << "1. Add Additional Part / Component\n";
                    std::cout << "2. Labor Charge (Current: RM " << std::fixed << std::setprecision(2) << v.laborCost << ")\n";
                    std::cout << "--- Logged Appointment Services & Parts (Parts Subtotal: RM " << std::fixed << std::setprecision(2) << v.partsCost << ") ---\n";

                    int optionNum = 3;
                    if (v.parts.empty()) {
                        std::cout << "   (No parts or appointment services logged)\n";
                    }
                    else {
                        for (size_t i = 0; i < v.parts.size(); ++i) {
                            std::cout << optionNum++ << ". " << v.parts[i].name
                                << " (RM " << std::fixed << std::setprecision(2) << v.parts[i].price << ")\n";
                        }
                    }
                    std::cout << "0. Save & Exit\n";
                    std::cout << "----------------------------------------\n";
                    std::cout << "Select item number to change (0 to finish): ";

                    if (!(std::cin >> choice)) {
                        std::cout << "Invalid input! Please enter a number.\n";
                        std::cin.clear();
                        std::cin.ignore(1000, '\n');
                        continue;
                    }

                    if (choice == 0) {
                        break;
                    }
                    else if (choice == 1) {
                        PartItem p;
                        int qty = 1;
                        double unitPrice = 0.0;

                        std::cout << "Enter Part Name: ";
                        std::cin.ignore(1000, '\n');
                        std::getline(std::cin, p.name);

                        while (true) {
                            std::cout << "Enter Quantity: ";
                            if (std::cin >> qty && qty > 0) break;
                            std::cout << "Invalid quantity!\n";
                            std::cin.clear();
                            std::cin.ignore(1000, '\n');
                        }

                        while (true) {
                            std::cout << "Enter Unit Price for " << p.name << ": RM ";
                            if (std::cin >> unitPrice && unitPrice >= 0.0) break;
                            std::cout << "Invalid price!\n";
                            std::cin.clear();
                            std::cin.ignore(1000, '\n');
                        }

                        if (qty > 1) {
                            p.name = std::to_string(qty) + "x " + p.name;
                        }
                        p.price = qty * unitPrice;
                        v.parts.push_back(p);
                    }
                    else if (choice == 2) {
                        double newLabor = 0.0;
                        while (true) {
                            std::cout << "Enter Labor Charge: RM ";
                            if (std::cin >> newLabor && newLabor >= 0.0) break;
                            std::cout << "Invalid labor charge!\n";
                            std::cin.clear();
                            std::cin.ignore(1000, '\n');
                        }
                        v.laborCost = newLabor;
                    }
                    else if (choice >= 3 && choice < 3 + static_cast<int>(v.parts.size())) {
                        int partIdx = choice - 3;
                        std::cout << "\n--- EDIT ITEM: " << v.parts[partIdx].name << " ---\n";
                        std::cout << "1. Edit Price\n";
                        std::cout << "2. Edit Description\n";
                        std::cout << "3. Remove Item\n";
                        std::cout << "Choice: ";

                        int subChoice = 0;
                        if (std::cin >> subChoice) {
                            if (subChoice == 1) {
                                double newPrice = 0.0;
                                std::cout << "Enter New Price: RM ";
                                if (std::cin >> newPrice && newPrice >= 0.0) {
                                    v.parts[partIdx].price = newPrice;
                                }
                            }
                            else if (subChoice == 2) {
                                std::cout << "Enter New Name: ";
                                std::cin.ignore(1000, '\n');
                                std::getline(std::cin, v.parts[partIdx].name);
                            }
                            else if (subChoice == 3) {
                                v.parts.erase(v.parts.begin() + partIdx);
                            }
                        }
                    }

                    // Recalculate Totals
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

                std::cout << "\n========================================\n";
                std::cout << "   FINAL UPDATED TOTALS FOR " << v.licensePlate << "\n";
                std::cout << "========================================\n";
                std::cout << "Parts Subtotal (Appointment Included) : RM " << std::fixed << std::setprecision(2) << v.partsCost << "\n";
                std::cout << "Labor Subtotal                         : RM " << v.laborCost << "\n";
                std::cout << "TOTAL COST                             : RM " << v.currentServiceCost << "\n";
                return;
            }
        }
    }
    std::cout << "Vehicle plate not found in database.\n";
}

void viewServiceBreakdown(const std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::string plate;
    std::cout << "\nEnter Vehicle License Plate (or 0 to Return): ";
    std::cin >> plate;
    if (plate == "0") return;

    plate = toUpper(plate);

    if (!hasActiveAppointment(plate, appointments)) {
        std::cout << "ERROR: Vehicle [" << plate << "] has no active appointment booking!\n";
        return;
    }

    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (toUpper(v.licensePlate) == plate) {
                std::cout << "\n========================================\n";
                std::cout << "        ITEMIZED SERVICE BREAKDOWN      \n";
                std::cout << "========================================\n";
                std::cout << "Vehicle Plate : " << v.licensePlate << " (" << v.brand << " " << v.model << ")\n";
                std::cout << "Owner         : " << c.name << " (Ph: " << c.phone << ")\n";
                std::cout << "Technician    : " << v.assignedTechnician << "\n";
                std::cout << "Status        : " << v.status << "\n";
                std::cout << "----------------------------------------\n";
                std::cout << "Appointment Items & Parts Log:\n";
                if (v.parts.empty()) {
                    std::cout << "  (No items logged)\n";
                }
                else {
                    for (size_t i = 0; i < v.parts.size(); ++i) {
                        std::cout << "  " << (i + 1) << ". " << v.parts[i].name
                            << " (RM " << std::fixed << std::setprecision(2) << v.parts[i].price << ")\n";
                    }
                }
                std::cout << "----------------------------------------\n";
                std::cout << std::fixed << std::setprecision(2);
                std::cout << "Parts Subtotal (Appointment Included) : RM " << v.partsCost << "\n";
                std::cout << "Labor Subtotal                         : RM " << v.laborCost << "\n";
                std::cout << "TOTAL ACCRUED                          : RM " << v.currentServiceCost << "\n";
                std::cout << "========================================\n";
                return;
            }
        }
    }
    std::cout << "Vehicle plate not found in database.\n";
}

void displayWorkshopVehicle(const std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::cout << "\n--- DASHBOARD VIEW OPTIONS ---\n";
    std::cout << "1. View All Vehicles with Active Appointments\n";
    std::cout << "2. Filter by Status (Awaiting Parts / In Progress / Ready)\n";
    std::cout << "3. Filter by Technician\n";
    std::cout << "Choice: ";

    int filterChoice = 1;
    if (!(std::cin >> filterChoice)) {
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        filterChoice = 1;
    }

    std::string targetStatus = "";
    std::string targetTech = "";

    if (filterChoice == 2) {
        std::cout << "1. Awaiting Parts | 2. In Progress | 3. Ready for Pickup: ";
        int sChoice;
        std::cin >> sChoice;
        if (sChoice == 1) targetStatus = "Awaiting Parts";
        else if (sChoice == 2) targetStatus = "In Progress";
        else if (sChoice == 3) targetStatus = "Ready for Pickup";
    }
    else if (filterChoice == 3) {
        std::cout << "Enter Technician Name to filter: ";
        std::cin.ignore(1000, '\n');
        std::getline(std::cin, targetTech);
    }

    std::cout << "\n----------------------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(12) << "Plate"
        << std::setw(16) << "Vehicle"
        << std::setw(18) << "Status"
        << std::setw(18) << "Technician"
        << std::setw(18) << "Owner"
        << "Accrued Cost (RM)\n";
    std::cout << "----------------------------------------------------------------------------------------------------\n";

    bool empty = true;
    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (!hasActiveAppointment(v.licensePlate, appointments)) {
                continue;
            }

            if (!targetStatus.empty() && v.status != targetStatus) continue;
            if (!targetTech.empty() && toUpper(v.assignedTechnician).find(toUpper(targetTech)) == std::string::npos) continue;

            empty = false;
            std::cout << std::left << std::setw(12) << v.licensePlate
                << std::setw(16) << (v.brand + " " + v.model)
                << std::setw(18) << v.status
                << std::setw(18) << v.assignedTechnician
                << std::setw(18) << c.name
                << std::fixed << std::setprecision(2) << v.currentServiceCost << "\n";
        }
    }
    if (empty) {
        std::cout << "No active appointment vehicles currently found in workshop.\n";
    }
}

void checkServiceReminders(const std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::cout << "\n=======================================================\n";
    std::cout << "        AUTOMATED SERVICE REMINDER DASHBOARD           \n";
    std::cout << "=======================================================\n";

    std::string today = getTodayDate();
    bool overdueFound = false;

    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (!hasActiveAppointment(v.licensePlate, appointments)) {
                continue;
            }

            if (v.lastServiceDate == "N/A" || v.lastServiceDate < today) {
                std::cout << "[OVERDUE FLAG] " << c.name << " (" << c.phone << ")"
                    << " | Car: " << v.licensePlate << " (" << v.brand << ")"
                    << " | Last Serviced: " << v.lastServiceDate << "\n";
                overdueFound = true;
            }
        }
    }
    if (!overdueFound) std::cout << "All active appointment vehicles are up to date on service schedules!\n";
}

void searchVehicleHistory(const std::vector<Customer>& customers) {
    std::string keyword;
    std::cout << "\nEnter keyword to search (Plate, Brand, or Model): ";
    std::cin.ignore(1000, '\n');
    std::getline(std::cin, keyword);
    keyword = toUpper(keyword);

    std::cout << "\n========================================\n";
    std::cout << "          VEHICLE SEARCH RESULTS         \n";
    std::cout << "========================================\n";

    bool found = false;
    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (toUpper(v.licensePlate).find(keyword) != std::string::npos ||
                toUpper(v.brand).find(keyword) != std::string::npos ||
                toUpper(v.model).find(keyword) != std::string::npos) {
                found = true;
                std::cout << "Plate         : " << v.licensePlate << "\n";
                std::cout << "Make/Model    : " << v.brand << " " << v.model << "\n";
                std::cout << "Owner         : " << c.name << " (Ph: " << c.phone << ")\n";
                std::cout << "Current Status: " << v.status << "\n";
                std::cout << "Last Serviced : " << v.lastServiceDate << "\n";
                std::cout << "----------------------------------------\n";
            }
        }
    }
    if (!found) {
        std::cout << "No matching vehicles found for keyword: " << keyword << "\n";
    }
}

void updateAllVehicleStatuses(std::vector<Customer>& customers, const std::vector<Appointment>& appointments) {
    std::string targetStatus;
    std::cout << "\n--- BULK UPDATE WORKSHOP STATUS ---\n";
    std::cout << "Select current status to target:\n";
    std::cout << "1. Awaiting Parts\n";
    std::cout << "2. In Progress\n";
    std::cout << "Choice: ";

    int choice;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        return;
    }

    if (choice == 1) targetStatus = "Awaiting Parts";
    else if (choice == 2) targetStatus = "In Progress";
    else {
        std::cout << "Invalid selection.\n";
        return;
    }

    std::cout << "Select new status to apply:\n";
    std::cout << "1. In Progress\n";
    std::cout << "2. Ready for Pickup\n";
    std::cout << "Choice: ";

    int newChoice;
    if (!(std::cin >> newChoice)) {
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        return;
    }

    std::string newStatus = "";
    if (newChoice == 1) newStatus = "In Progress";
    else if (newChoice == 2) newStatus = "Ready for Pickup";
    else {
        std::cout << "Invalid selection.\n";
        return;
    }

    int updatedCount = 0;
    for (auto& c : customers) {
        for (auto& v : c.vehicles) {
            if (hasActiveAppointment(v.licensePlate, appointments) && v.status == targetStatus) {
                v.status = newStatus;
                if (newStatus == "Ready for Pickup") {
                    v.lastServiceDate = getTodayDate();
                    if (v.currentServiceCost == 0.0) {
                        syncAppointmentToPartsCost(v, appointments);
                    }
                }
                updatedCount++;
            }
        }
    }

    std::cout << "Successfully updated " << updatedCount << " vehicle(s) from ["
        << targetStatus << "] to [" << newStatus << "]!\n";
}

void resetVehicleServiceData(Vehicle& vehicle) {
    vehicle.parts.clear();
    vehicle.activeServiceLogs.clear();
    vehicle.partsCost = 0.0;
    vehicle.laborCost = 0.0;
    vehicle.currentServiceCost = 0.0;
    vehicle.status = "Completed";
}