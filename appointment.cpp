#include "appointment.h"
#include "customer.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <sstream>
#include <limits>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <vector>

static void manageAppointmentFromList(std::vector<Appointment*>& foundAppointments,
    std::vector<Appointment>& allAppointments);
static void rescheduleSpecificAppointment(Appointment* app, std::vector<Appointment>& allAppointments);
static void cancelSpecificAppointment(Appointment* app);

// Define hierarchical structures with fixed service fees
struct SubOption {
    std::string name;
    std::string description;
    double fee; // Fixed Service Fee
};

struct OptionCategory {
    std::string categoryName;
    std::vector<SubOption> subOptions;
};

struct MainBookingType {
    std::string typeName;
    std::vector<OptionCategory> categories;
};

// Track selected item with its associated fee
struct SelectedItem {
    std::string fullDescription;
    double fee;
};

// Static tracker for session maximum ID
static int highestAppIdEver = 0;

// Helper function to generate sequential APP0001 formatted ID without reusing deleted IDs
static std::string generateNextAppointmentId(const std::vector<Appointment>& appointments) {
    int maxId = highestAppIdEver;

    // 1. Check current active/loaded appointments
    for (const auto& app : appointments) {
        if (app.appointmentId.length() > 3 && app.appointmentId.substr(0, 3) == "APP") {
            try {
                int num = std::stoi(app.appointmentId.substr(3));
                if (num > maxId) maxId = num;
            }
            catch (...) {}
        }
    }

    // 2. Check appointments.txt history file to prevent ID collisions
    std::ifstream appFile("appointments.txt");
    if (appFile.is_open()) {
        std::string line;
        while (std::getline(appFile, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string appId;
            std::getline(ss, appId, '|');
            if (appId.length() > 3 && appId.substr(0, 3) == "APP") {
                try {
                    int num = std::stoi(appId.substr(3));
                    if (num > maxId) maxId = num;
                }
                catch (...) {}
            }
        }
        appFile.close();
    }

    int nextNum = maxId + 1;
    highestAppIdEver = nextNum;

    std::stringstream ss;
    ss << "APP" << std::setw(4) << std::setfill('0') << nextNum;
    return ss.str();
}

// Verification Helper: Converts all characters in a string to uppercase
static std::string toUpper(std::string str) {
    for (char& c : str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return str;
}

// Verification Helper: Validates YYYY-MM-DD format & actual calendar days
static bool isValidDate(const std::string& date) {
    if (date.length() != 10) return false;
    if (date[4] != '-' || date[7] != '-') return false;

    for (int i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;
    }

    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));

    if (year < 2000 || year > 2100) return false;
    if (month < 1 || month > 12) return false;

    int daysInMonth[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    // Leap year calculation
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[2] = 29;
    }

    return (day >= 1 && day <= daysInMonth[month]);
}

// Verification Helper: Validates HH:MM format (24-hour clock)
static bool isValidTime(const std::string& timeStr) {
    if (timeStr.length() != 5) return false;
    if (timeStr[2] != ':') return false;

    for (int i = 0; i < 5; ++i) {
        if (i == 2) continue;
        if (!std::isdigit(static_cast<unsigned char>(timeStr[i]))) return false;
    }

    int hour = std::stoi(timeStr.substr(0, 2));
    int minute = std::stoi(timeStr.substr(3, 2));

    return (hour >= 0 && hour <= 23) && (minute >= 0 && minute <= 59);
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

static void displayAppointmentRow(const Appointment& a) {
    std::cout << std::left << std::setw(10) << a.appointmentId
        << std::setw(12) << a.customerId
        << std::setw(12) << a.licensePlate
        << std::setw(14) << a.date
        << std::setw(10) << a.time
        << (a.isActive ? "Active" : "Cancelled") << "\n";
}

static void displayAppointmentHeader() {
    std::cout << "\n----------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(5) << "No."
        << std::setw(10) << "Appt ID"
        << std::setw(12) << "Cust ID"
        << std::setw(12) << "Plate"
        << std::setw(14) << "Date"
        << std::setw(10) << "Time"
        << "Status\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
}

void appointmentMenu(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "         APPOINTMENT MANAGEMENT         \n";
        std::cout << "========================================\n";
        std::cout << "1. Book Appointment\n";
        std::cout << "2. View Appointments\n";
        std::cout << "3. Search Appointment\n";
        std::cout << "4. Reschedule Appointment\n";
        std::cout << "5. Cancel Appointment\n";
        std::cout << "6. Filter by Service Type Description\n";
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
            bookAppointment(appointments, customers);
            saveAppointmentsToFile(appointments);
            break;
        case 2:
            viewAppointmentsMenu(appointments);
            break;
        case 3:
            searchAppointment(appointments);
            break;
        case 4:
            rescheduleAppointment(appointments);
            saveAppointmentsToFile(appointments);
            break;
        case 5:
            cancelAppointment(appointments);
            saveAppointmentsToFile(appointments);
            break;
        case 6:
            filterAppointmentsByServiceType(appointments);
            break;
        case 0:
            std::cout << "Returning to Main Menu...\n";
            break;
        default:
            std::cout << "Invalid choice! Please select an option between 0 and 6.\n";
        }
    } while (choice != 0);
}

void bookAppointment(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
    Appointment app;
    app.appointmentId = "";
    app.customerId = "";
    app.licensePlate = "";
    app.date = "";
    app.time = "";
    app.serviceType = "";
    app.isActive = true;

    std::cout << "\nEnter Customer ID (or 0 to Return to Main Menu): ";
    std::cin >> app.customerId;
    if (app.customerId == "0") return;

    app.customerId = toUpper(app.customerId);

    Customer* targetCustomer = nullptr;
    for (auto& c : customers) {
        if (c.id == app.customerId) {
            targetCustomer = &c;
            break;
        }
    }
    if (!targetCustomer) {
        std::cout << "ERROR: Customer ID not found!\n";
        return;
    }

    std::cout << "\nCustomer Found: " << targetCustomer->name << " (" << targetCustomer->id << ")\n";

    std::string selectedPlate;
    while (true) {
        std::cout << "\n----------------------------------------\n";
        std::cout << "SELECT VEHICLE FOR APPOINTMENT:\n";
        std::cout << "----------------------------------------\n";

        int optionNumber = 1;
        for (const auto& v : targetCustomer->vehicles) {
            std::cout << optionNumber << ". " << v.licensePlate
                << " (" << v.brand << " " << v.model << ")\n";
            optionNumber++;
        }
        std::cout << optionNumber << ". [+ Add New Vehicle]\n";
        std::cout << "0. Cancel Appointment\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Choice: ";

        int vChoice;
        if (!(std::cin >> vChoice)) {
            std::cout << "Invalid input! Enter a valid menu option number.\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        if (vChoice == 0) {
            std::cout << "Appointment booking cancelled.\n";
            return;
        }

        if (vChoice >= 1 && vChoice <= static_cast<int>(targetCustomer->vehicles.size())) {
            selectedPlate = targetCustomer->vehicles[vChoice - 1].licensePlate;
            break;
        }
        else if (vChoice == optionNumber) {
            Vehicle newV;

            while (true) {
                std::cout << "\nEnter New Vehicle License Plate: ";
                std::cin >> newV.licensePlate;
                newV.licensePlate = toUpper(newV.licensePlate);

                if (!isValidLicensePlate(newV.licensePlate)) {
                    std::cout << "ERROR: License plate must contain at least 1 letter, at least 1 digit, and no spaces/special characters!\n";
                }
                else {
                    break;
                }
            }

            std::cin.ignore(1000, '\n');
            std::cout << "Enter Vehicle Brand (e.g. Honda): ";
            std::getline(std::cin, newV.brand);
            std::cout << "Enter Vehicle Model (e.g. Civic): ";
            std::getline(std::cin, newV.model);

            newV.lastServiceDate = "N/A";
            newV.status = "In Service Queue";
            newV.assignedTechnician = "Unassigned";
            newV.partsCost = 0.0;
            newV.laborCost = 0.0;
            newV.currentServiceCost = 0.0;

            targetCustomer->vehicles.push_back(newV);
            saveCustomersToFile(customers);

            selectedPlate = newV.licensePlate;
            std::cout << "\nVehicle [" << selectedPlate << "] registered and selected successfully!\n";
            break;
        }
        else {
            std::cout << "Invalid selection! Please select a number from the menu.\n";
        }
    }

    app.licensePlate = toUpper(selectedPlate);

    std::string today = getTodayDate();
    while (true) {
        std::cout << "Enter Service Date (YYYY-MM-DD, Today or Future): ";
        std::cin >> app.date;
        if (!isValidDate(app.date)) {
            std::cout << "ERROR: Invalid date format or calendar day! Use YYYY-MM-DD.\n";
        }
        else if (app.date < today) {
            std::cout << "ERROR: Appointment date cannot be in the past! Current date is: " << today << "\n";
        }
        else {
            break;
        }
    }

    while (true) {
        std::cout << "Enter Time Slot (HH:MM in 24-hr format): ";
        std::cin >> app.time;
        if (!isValidTime(app.time)) {
            std::cout << "ERROR: Invalid time format! Use HH:MM (00:00 to 23:59).\n";
        }
        else {
            break;
        }
    }

    for (const auto& a : appointments) {
        if (a.isActive && a.date == app.date && a.time == app.time) {
            std::cout << "ERROR: Slot already booked!\n";
            return;
        }
    }

    // --- TWO-TIERED BOOKING CATALOG WITH FIXED SERVICE FEES ---
    std::vector<MainBookingType> mainTypes = {
        {
            "Car Problem / Repair Request",
            {
                {
                    "Engine & Driveability Issues",
                    {
                        {"Engine Stalling / Rough Idle", "Vehicle stalls, struggles to start, or idling is unstable.", 120.00},
                        {"Check Engine Light Diagnostic", "OBD scanner diagnosis & troubleshooting.", 80.00},
                        {"Engine Overheating Repair", "Coolant leak check & thermostat diagnostic.", 150.00}
                    }
                },
                {
                    "Brake, Steering & Suspension Issues",
                    {
                        {"Brake Noise / Low Power Diagnostic", "Inspection of brake pads, rotors, and fluid.", 60.00},
                        {"Steering Pull & Suspension Check", "Wheel alignment & suspension arm inspection.", 90.00}
                    }
                },
                {
                    "Electrical & Air Conditioning Issues",
                    {
                        {"Battery / Alternator Test & Repair", "Battery health check & alternator output test.", 50.00},
                        {"Aircon Pressure & Cooling Repair", "AC gas pressure test & leak check.", 100.00},
                        {"Electrical Accessories Fault Check", "Wiring, fuses, and window motor repair.", 70.00}
                    }
                }
            }
        },
        {
            "Scheduled Maintenance / Servicing",
            {
                {
                    "Interim Maintenance",
                    {
                        {"Basic Engine Oil & Filter Change", "Engine oil drain/fill and new oil filter replacement.", 130.00},
                        {"Safety Inspection & Fluid Top-Up", "Check of brakes, lights, coolant, wiper fluid.", 50.00}
                    }
                },
                {
                    "Full Periodic Service",
                    {
                        {"Standard Comprehensive Service", "Complete system checks, spark plug check, air filter clean.", 250.00},
                        {"Brake System Service & Flush", "Brake pad cleaning, slider lube, brake fluid flush.", 140.00}
                    }
                },
                {
                    "Major Overhaul Maintenance",
                    {
                        {"Major Service & Timing Belt Package", "Comprehensive service with timing belt/chain replacement.", 650.00},
                        {"Transmission & Gearbox Fluid Service", "Transmission fluid flush & filter change.", 220.00}
                    }
                }
            }
        }
    };

    // --- MULTI-SELECTION LOOP FOR PROBLEMS & SERVICES WITH COST CALCULATION ---
    std::vector<SelectedItem> selectedItems;
    double totalServiceFee = 0.0;
    bool doneSelecting = false;

    while (!doneSelecting) {
        std::cout << "\n========================================================================\n";
        std::cout << "                  SELECT REASON(S) FOR APPOINTMENT                      \n";
        std::cout << "========================================================================\n";

        if (!selectedItems.empty()) {
            std::cout << "CURRENTLY SELECTED (" << selectedItems.size() << " item(s)):\n";
            for (size_t i = 0; i < selectedItems.size(); ++i) {
                std::cout << "  " << (i + 1) << ". " << selectedItems[i].fullDescription
                    << " [Fee: $" << std::fixed << std::setprecision(2) << selectedItems[i].fee << "]\n";
            }
            std::cout << "  TOTAL ESTIMATED SERVICE FEE: $" << std::fixed << std::setprecision(2) << totalServiceFee << "\n";
            std::cout << "------------------------------------------------------------------------\n";
        }

        std::cout << "1. Add a Car Problem / Repair Request\n";
        std::cout << "2. Add Scheduled Maintenance / Servicing\n";
        if (!selectedItems.empty()) {
            std::cout << "3. [DONE] Finish Selection and Proceed ($" << std::fixed << std::setprecision(2) << totalServiceFee << " total)\n";
        }
        std::cout << "0. Cancel Booking\n";
        std::cout << "------------------------------------------------------------------------\n";
        std::cout << "Select Option: ";

        int mainChoice;
        if (!(std::cin >> mainChoice)) {
            std::cout << "Invalid input! Please enter a number.\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        if (mainChoice == 0) {
            std::cout << "Appointment booking cancelled.\n";
            return;
        }

        if (mainChoice == 3 && !selectedItems.empty()) {
            doneSelecting = true;
            break;
        }

        if (mainChoice == 1 || mainChoice == 2) {
            const auto& selectedMain = mainTypes[mainChoice - 1];

            while (true) {
                std::cout << "\n========================================================================\n";
                std::cout << "    CATEGORIES FOR: " << selectedMain.typeName << "\n";
                std::cout << "========================================================================\n";
                for (size_t c = 0; c < selectedMain.categories.size(); ++c) {
                    std::cout << (c + 1) << ". " << selectedMain.categories[c].categoryName << "\n";
                }
                std::cout << "0. Back to Main Selection\n";
                std::cout << "------------------------------------------------------------------------\n";
                std::cout << "Select Category (1 - " << selectedMain.categories.size() << "): ";

                int catChoice;
                if (!(std::cin >> catChoice)) {
                    std::cout << "Invalid input! Please enter a number.\n";
                    std::cin.clear();
                    std::cin.ignore(1000, '\n');
                    continue;
                }

                if (catChoice == 0) break; // Back to main selection loop

                if (catChoice >= 1 && catChoice <= static_cast<int>(selectedMain.categories.size())) {
                    const auto& selectedCat = selectedMain.categories[catChoice - 1];

                    while (true) {
                        std::cout << "\n------------------------------------------------------------------------\n";
                        std::cout << "    SPECIFIC OPTIONS FOR: " << selectedCat.categoryName << "\n";
                        std::cout << "------------------------------------------------------------------------\n";
                        for (size_t s = 0; s < selectedCat.subOptions.size(); ++s) {
                            std::cout << (s + 1) << ". " << selectedCat.subOptions[s].name
                                << " [Fee: $" << std::fixed << std::setprecision(2) << selectedCat.subOptions[s].fee << "]\n"
                                << "    Desc: " << selectedCat.subOptions[s].description << "\n";
                            std::cout << "------------------------------------------------------------------------\n";
                        }
                        std::cout << "0. Back to Categories\n";
                        std::cout << "Select Specific Issue / Service to ADD: ";

                        int subChoice;
                        if (!(std::cin >> subChoice)) {
                            std::cout << "Invalid input! Please enter a number.\n";
                            std::cin.clear();
                            std::cin.ignore(1000, '\n');
                            continue;
                        }

                        if (subChoice == 0) break; // Back to category selection

                        if (subChoice >= 1 && subChoice <= static_cast<int>(selectedCat.subOptions.size())) {
                            const auto& sub = selectedCat.subOptions[subChoice - 1];
                            std::string itemStr = "[" + selectedMain.typeName + "] " + selectedCat.categoryName + " -> " + sub.name;

                            // Duplicate check
                            bool exists = false;
                            for (const auto& item : selectedItems) {
                                if (item.fullDescription == itemStr) {
                                    exists = true;
                                    break;
                                }
                            }

                            if (exists) {
                                std::cout << "\n>>> NOTE: This item is already in your selection list! <<<\n";
                            }
                            else {
                                selectedItems.push_back({ itemStr, sub.fee });
                                totalServiceFee += sub.fee;
                                std::cout << "\n>>> ADDED SUCCESSFULLY: " << itemStr << " ($" << std::fixed << std::setprecision(2) << sub.fee << ") <<<\n";
                            }
                            break; // Return to category level after adding item
                        }
                        else {
                            std::cout << "Invalid selection! Try again.\n";
                        }
                    }
                }
                else {
                    std::cout << "Invalid category selection! Try again.\n";
                }
            }
        }
        else {
            std::cout << "Invalid selection! Please choose a valid option.\n";
        }
    }

    // --- CONCATENATE ALL SELECTED SERVICES INTO ONE STRING WITH COST DETAILS ---
    std::string combinedServices = "";
    for (size_t i = 0; i < selectedItems.size(); ++i) {
        std::stringstream feeStream;
        feeStream << std::fixed << std::setprecision(2) << selectedItems[i].fee;

        combinedServices += "(" + std::to_string(i + 1) + ") " + selectedItems[i].fullDescription + " [$" + feeStream.str() + "]";
        if (i < selectedItems.size() - 1) {
            combinedServices += " | ";
        }
    }

    // Append total fixed fee summary
    std::stringstream totalStream;
    totalStream << std::fixed << std::setprecision(2) << totalServiceFee;
    combinedServices += " {Total Service Fee: $" + totalStream.str() + "}";

    // --- OPTIONAL REMARK / NOTE INPUT ---
    std::cin.ignore(1000, '\n');
    std::string remarkInput = "";
    std::cout << "\nEnter Additional Remarks / Symptoms Description (Press Enter to skip): ";
    std::getline(std::cin, remarkInput);

    if (!remarkInput.empty()) {
        app.serviceType = combinedServices + " [Remark: " + remarkInput + "]";
    }
    else {
        app.serviceType = combinedServices;
    }

    // --- PASS COST AND STATUS DATA TO VEHICLE PART & SAVE CUSTOMER DATA ---
    for (auto& v : targetCustomer->vehicles) {
        if (v.licensePlate == app.licensePlate) {
            v.laborCost += totalServiceFee;          // Assign service fees to labor/service cost
            v.currentServiceCost += totalServiceFee; // Update cumulative current service bill
            v.status = "In Service Queue";          // Update vehicle workflow status
            break;
        }
    }
    saveCustomersToFile(customers); // Sync updated vehicle charges to storage file for Billing module

    app.appointmentId = generateNextAppointmentId(appointments);
    app.isActive = true;
    appointments.push_back(app);

    std::cout << "\nBooking successful! Vehicle service cost updated and passed to Billing.\n";
    generateBookingSlip(app);
}

void generateBookingSlip(const Appointment& app) {
    std::cout << "\n----------------------------------------\n";
    std::cout << "         APPOINTMENT CONFIRMATION       \n";
    std::cout << "----------------------------------------\n";
    std::cout << "Booking ID    : " << app.appointmentId << "\n";
    std::cout << "Customer ID   : " << app.customerId << "\n";
    std::cout << "Vehicle Plate : " << app.licensePlate << "\n";
    std::cout << "Schedule      : " << app.date << " @ " << app.time << "\n";
    std::cout << "Services/Fees : " << app.serviceType << "\n";
    std::cout << "Status        : " << (app.isActive ? "CONFIRMED" : "CANCELLED") << "\n";
    std::cout << "----------------------------------------\n";
}

void viewAppointmentsMenu(std::vector<Appointment>& appointments) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "            VIEW APPOINTMENTS           \n";
        std::cout << "========================================\n";
        std::cout << "1. View All Appointments (Sorted by Appt ID)\n";
        std::cout << "2. View Today's Appointments (Sorted by Time)\n";
        std::cout << "3. View Appointments by Date (Sorted by Time)\n";
        std::cout << "0. Return to Appointment Menu\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Enter choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input!\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        switch (choice) {
        case 1:
            viewAllAppointments(appointments);
            break;
        case 2:
            viewTodaysAppointments(appointments);
            break;
        case 3:
            viewAppointmentsByDate(appointments);
            break;
        case 0:
            std::cout << "Returning to Appointment Menu...\n";
            break;
        default:
            std::cout << "Invalid choice!\n";
        }
    } while (choice != 0);
}

void viewAllAppointments(std::vector<Appointment>& appointments) {
    if (appointments.empty()) {
        std::cout << "\nNo appointments found.\n";
        return;
    }

    std::vector<Appointment*> foundAppointments;
    for (auto& a : appointments) {
        foundAppointments.push_back(&a);
    }

    std::sort(foundAppointments.begin(), foundAppointments.end(), [](const Appointment* a, const Appointment* b) {
        return a->appointmentId < b->appointmentId;
        });

    displayAppointmentHeader();
    int index = 1;
    for (auto* a : foundAppointments) {
        std::cout << std::left << std::setw(5) << (std::to_string(index++) + ".");
        displayAppointmentRow(*a);
    }

    manageAppointmentFromList(foundAppointments, appointments);
}

void viewAppointmentsByDate(std::vector<Appointment>& appointments) {
    std::string date;
    while (true) {
        std::cout << "\nEnter Date to view (YYYY-MM-DD, or 0 to Return): ";
        std::cin >> date;
        if (date == "0") return;

        if (!isValidDate(date)) {
            std::cout << "ERROR: Invalid date format! Use YYYY-MM-DD.\n";
        }
        else {
            break;
        }
    }

    std::vector<Appointment*> foundAppointments;
    for (auto& a : appointments) {
        if (a.date == date) {
            foundAppointments.push_back(&a);
        }
    }

    if (foundAppointments.empty()) {
        std::cout << "No appointments found for date: " << date << "\n";
        return;
    }

    std::sort(foundAppointments.begin(), foundAppointments.end(), [](const Appointment* a, const Appointment* b) {
        return a->time < b->time;
        });

    displayAppointmentHeader();
    int index = 1;
    for (auto* a : foundAppointments) {
        std::cout << std::left << std::setw(5) << (std::to_string(index++) + ".");
        displayAppointmentRow(*a);
    }

    manageAppointmentFromList(foundAppointments, appointments);
}

void viewTodaysAppointments(std::vector<Appointment>& appointments) {
    std::string today = getTodayDate();
    std::cout << "\n=== TODAY'S APPOINTMENTS (" << today << ") ===\n";

    std::vector<Appointment*> foundAppointments;
    for (auto& a : appointments) {
        if (a.date == today) {
            foundAppointments.push_back(&a);
        }
    }

    if (foundAppointments.empty()) {
        std::cout << "No appointments scheduled for today.\n";
        return;
    }

    std::sort(foundAppointments.begin(), foundAppointments.end(), [](const Appointment* a, const Appointment* b) {
        return a->time < b->time;
        });

    displayAppointmentHeader();
    int index = 1;
    for (auto* a : foundAppointments) {
        std::cout << std::left << std::setw(5) << (std::to_string(index++) + ".");
        displayAppointmentRow(*a);
    }

    manageAppointmentFromList(foundAppointments, appointments);
}

static void manageAppointmentFromList(std::vector<Appointment*>& foundAppointments,
    std::vector<Appointment>& allAppointments) {
    if (foundAppointments.empty()) return;

    std::cout << "\nDo you want to manage an appointment from this list?\n";
    std::cout << "1. Reschedule an appointment\n";
    std::cout << "2. Cancel an appointment\n";
    std::cout << "0. Return to menu\n";
    std::cout << "Choice: ";

    int choice;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        return;
    }

    if (choice == 0) return;

    if (choice == 1 || choice == 2) {
        int appointmentIndex;
        std::cout << "Enter the number (#) of the appointment to "
            << (choice == 1 ? "reschedule" : "cancel")
            << " (1-" << foundAppointments.size() << "): ";

        if (!(std::cin >> appointmentIndex) || appointmentIndex < 1 ||
            appointmentIndex > static_cast<int>(foundAppointments.size())) {
            std::cout << "Invalid selection!\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            return;
        }

        Appointment* selectedApp = foundAppointments[appointmentIndex - 1];

        if (choice == 1) {
            rescheduleSpecificAppointment(selectedApp, allAppointments);
        }
        else {
            cancelSpecificAppointment(selectedApp);
            saveAppointmentsToFile(allAppointments);
        }
    }
    else {
        std::cout << "Invalid choice!\n";
    }
}

static void rescheduleSpecificAppointment(Appointment* app, std::vector<Appointment>& allAppointments) {
    if (!app || !app->isActive) {
        std::cout << "This appointment is not active or does not exist.\n";
        return;
    }

    std::string newDate, newTime;
    std::string today = getTodayDate();
    std::cout << "\nCurrent Schedule: " << app->date << " @ " << app->time << "\n";

    while (true) {
        std::cout << "Enter New Date (YYYY-MM-DD, Today or Future): ";
        std::cin >> newDate;
        if (!isValidDate(newDate)) {
            std::cout << "ERROR: Invalid date format or calendar day! Use YYYY-MM-DD.\n";
        }
        else if (newDate < today) {
            std::cout << "ERROR: Rescheduled date cannot be in the past! Current date is: " << today << "\n";
        }
        else {
            break;
        }
    }

    while (true) {
        std::cout << "Enter New Time (HH:MM in 24-hr format): ";
        std::cin >> newTime;
        if (!isValidTime(newTime)) {
            std::cout << "ERROR: Invalid time format! Use HH:MM (00:00 to 23:59).\n";
        }
        else {
            break;
        }
    }

    for (const auto& a : allAppointments) {
        if (&a != app && a.isActive && a.date == newDate && a.time == newTime) {
            std::cout << "ERROR: Destination slot is already occupied!\n";
            return;
        }
    }

    std::string oldDate = app->date;
    std::string oldTime = app->time;
    app->date = newDate;
    app->time = newTime;
    std::cout << "Appointment " << app->appointmentId
        << " rescheduled from " << oldDate << " @ " << oldTime
        << " to " << newDate << " @ " << newTime << " successfully!\n";

    saveAppointmentsToFile(allAppointments);
}

static void cancelSpecificAppointment(Appointment* app) {
    if (!app) {
        std::cout << "Appointment not found.\n";
        return;
    }

    if (!app->isActive) {
        std::cout << "This appointment is already cancelled.\n";
        return;
    }

    app->isActive = false;
    std::cout << "Appointment " << app->appointmentId << " successfully cancelled.\n";
}

void searchAppointment(const std::vector<Appointment>& appointments) {
    std::string query;
    std::cout << "\nEnter Booking ID or Customer ID (or 0 to Return to Main Menu): ";
    std::cin >> query;
    if (query == "0") return;

    query = toUpper(query);

    bool found = false;
    for (const auto& a : appointments) {
        if (a.appointmentId == query || a.customerId == query) {
            generateBookingSlip(a);
            found = true;
        }
    }
    if (!found) std::cout << "No matching appointments found.\n";
}

void rescheduleAppointment(std::vector<Appointment>& appointments) {
    std::cout << "\n--- Active Appointments ---\n";

    std::vector<Appointment*> activeApps;
    for (auto& a : appointments) {
        if (a.isActive) {
            activeApps.push_back(&a);
        }
    }

    if (activeApps.empty()) {
        std::cout << "No active appointments found.\n";
        return;
    }

    std::sort(activeApps.begin(), activeApps.end(), [](const Appointment* a, const Appointment* b) {
        return a->appointmentId < b->appointmentId;
        });

    displayAppointmentHeader();
    int index = 1;
    for (const auto* a : activeApps) {
        std::cout << std::left << std::setw(5) << (std::to_string(index++) + ".");
        displayAppointmentRow(*a);
    }

    std::string appId;
    std::cout << "\nEnter Appointment ID to reschedule (or 0 to Return to Main Menu): ";
    std::cin >> appId;
    if (appId == "0") return;

    appId = toUpper(appId);

    for (auto& a : appointments) {
        if (a.appointmentId == appId && a.isActive) {
            rescheduleSpecificAppointment(&a, appointments);
            return;
        }
    }
    std::cout << "Active appointment ID not found.\n";
}

void cancelAppointment(std::vector<Appointment>& appointments) {
    std::cout << "\n--- Active Appointments ---\n";

    std::vector<Appointment*> activeApps;
    for (auto& a : appointments) {
        if (a.isActive) {
            activeApps.push_back(&a);
        }
    }

    if (activeApps.empty()) {
        std::cout << "No active appointments found.\n";
        return;
    }

    std::sort(activeApps.begin(), activeApps.end(), [](const Appointment* a, const Appointment* b) {
        return a->appointmentId < b->appointmentId;
        });

    displayAppointmentHeader();
    int index = 1;
    for (const auto* a : activeApps) {
        std::cout << std::left << std::setw(5) << (std::to_string(index++) + ".");
        displayAppointmentRow(*a);
    }

    std::string appId;
    std::cout << "\nEnter Appointment ID to cancel (or 0 to Return to Main Menu): ";
    std::cin >> appId;
    if (appId == "0") return;

    appId = toUpper(appId);

    for (auto& a : appointments) {
        if (a.appointmentId == appId) {
            cancelSpecificAppointment(&a);
            saveAppointmentsToFile(appointments);
            return;
        }
    }
    std::cout << "Appointment ID not found.\n";
}

void filterAppointmentsByServiceType(const std::vector<Appointment>& appointments) {
    std::string keyword;
    std::cout << "\nEnter service category or keyword to filter (e.g. Repair, Maintenance, Engine, Brake, Oil): ";
    std::cin.ignore(1000, '\n');
    std::getline(std::cin, keyword);
    keyword = toUpper(keyword);

    std::cout << "\n========================================\n";
    std::cout << "       SERVICE TYPE FILTER RESULTS      \n";
    std::cout << "========================================\n";

    bool found = false;
    displayAppointmentHeader();
    int index = 1;
    for (const auto& a : appointments) {
        std::string serviceUpper = toUpper(a.serviceType);
        if (serviceUpper.find(keyword) != std::string::npos) {
            found = true;
            std::cout << std::left << std::setw(5) << (std::to_string(index++) + ".");
            displayAppointmentRow(a);
            std::cout << "    -> Details: " << a.serviceType << "\n";
        }
    }

    if (!found) {
        std::cout << "No appointments matched the service keyword: " << keyword << "\n";
    }
    std::cout << "----------------------------------------------------------------------------------------\n";
}