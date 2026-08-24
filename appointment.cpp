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
    std::vector<Appointment>& allAppointments, std::vector<Customer>& customers);
static void rescheduleSpecificAppointment(Appointment* app, std::vector<Appointment>& allAppointments);
static void cancelSpecificAppointment(Appointment* app, std::vector<Customer>& customers);

// Define hierarchical structures - Problems have NO price, Services HAVE price
struct SubOption {
    std::string name;
    std::string description;
    double fee; // 0 for problems, actual price for services
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

// Helper function to convert string to uppercase
static std::string toUpper(std::string str) {
    for (char& c : str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return str;
}

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
        << (a.isActive ? "Active" : "Inactive") << "\n";
}

static void displayAppointmentHeader() {
    std::cout << "\n----------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(5) << "No."
        << std::setw(10) << "Appt ID"
        << std::setw(12) << "Phone"
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
            viewAppointmentsMenu(appointments, customers);
            break;
        case 3:
            searchAppointment(appointments);
            break;
        case 4:
            rescheduleAppointment(appointments);
            saveAppointmentsToFile(appointments);
            break;
        case 5:
            cancelAppointment(appointments, customers);
            saveAppointmentsToFile(appointments);
            break;
        case 0:
            std::cout << "Returning to Main Menu...\n";
            break;
        default:
            std::cout << "Invalid choice! Please select an option between 0 and 5.\n";
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
    app.totalFee = 0.0;
    app.isActive = true;

    std::cout << "\nEnter Customer Phone Number (or 0 to Return to Main Menu): ";
    std::cin >> app.customerId;
    if (app.customerId == "0") return;

    app.customerId = toUpper(app.customerId);

    Customer* targetCustomer = nullptr;
    for (auto& c : customers) {
        if (c.phone == app.customerId) {
            targetCustomer = &c;
            break;
        }
    }
    if (!targetCustomer) {
        std::cout << "ERROR: Customer phone number not found!\n";
        return;
    }

    std::cout << "\nCustomer Found: " << targetCustomer->name << " (" << targetCustomer->phone << ")\n";

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
                else if (isPlateTaken(customers, newV.licensePlate)) {
                    std::cout << "ERROR: This license plate is already registered to another customer!\n";
                }
                else {
                    break;
                }
            }

            std::cin.ignore(1000, '\n');
            std::cout << "Enter Vehicle Brand (e.g. Honda): ";
            std::getline(std::cin, newV.brand);
            newV.brand = toUpper(newV.brand);

            std::cout << "Enter Vehicle Model (e.g. Civic): ";
            std::getline(std::cin, newV.model);
            newV.model = toUpper(newV.model);

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

    // --- ENFORCE: only ONE active appointment allowed per vehicle at a time ---
    for (const auto& a : appointments) {
        if (a.isActive && a.licensePlate == app.licensePlate) {
            std::cout << "\nERROR: This vehicle [" << app.licensePlate << "] already has an active appointment ("
                << a.appointmentId << ", " << a.date << " @ " << a.time << ").\n";
            std::cout << "Please reschedule/cancel the existing appointment before booking a new one.\n";
            return;
        }
    }

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

    // --- TWO-TIERED BOOKING CATALOG - Problems (NO price) vs Services (WITH price) ---
    std::vector<MainBookingType> mainTypes = {
        {
            "Car Problems / Repair Issues",
            {
                {
                    "Engine Problems",
                    {
                        {"Engine Cannot Start", "Vehicle fails to start or crank.", 0.00},
                        {"Engine Stalling / Rough Idle", "Engine dies or runs unevenly.", 0.00},
                        {"Engine Overheating", "Temperature gauge showing high reading.", 0.00},
                        {"Loss of Engine Power", "Vehicle feels sluggish or lacks acceleration.", 0.00},
                        {"Knocking / Ticking Noises", "Unusual sounds from engine compartment.", 0.00},
                        {"Smoke from Exhaust", "Excessive smoke (white, blue, or black).", 0.00}
                    }
                },
                {
                    "Brake Problems",
                    {
                        {"Squealing / Grinding Brakes", "Noise when braking - pads or rotors worn.", 0.00},
                        {"Brake Pedal Soft / Spongy", "Brake pedal feels spongy or sinks to floor.", 0.00},
                        {"Brake Warning Light On", "Brake system warning light illuminated.", 0.00},
                        {"Car Pulls When Braking", "Vehicle pulls to one side during braking.", 0.00}
                    }
                },
                {
                    "Tyre & Wheel Problems",
                    {
                        {"Punctured / Flat Tyre", "Tyre has a puncture or completely deflated.", 0.00},
                        {"Bald / Worn Tyres", "Tyre tread worn below legal limit.", 0.00},
                        {"Tyre Pressure Warning", "TPMS warning light is on dashboard.", 0.00},
                        {"Uneven Tyre Wear", "Tyres wearing out faster on one side.", 0.00},
                        {"Wheel Alignment Problem", "Vehicle pulls or steering off-center.", 0.00},
                        {"Wheel Balancing Issue", "Vibration felt at certain speeds.", 0.00}
                    }
                },
                {
                    "Steering & Suspension Problems",
                    {
                        {"Car Pulls to One Side", "Vehicle drifts left or right while driving.", 0.00},
                        {"Bumpy / Unstable Ride", "Excessive bouncing or instability on road.", 0.00},
                        {"Steering Wheel Vibration", "Vibration felt through steering wheel.", 0.00},
                        {"Steering Noise / Hard to Turn", "Noise when turning or steering wheel stiff.", 0.00}
                    }
                },
                {
                    "Electrical Problems",
                    {
                        {"Battery Dead / Won't Hold Charge", "Battery needs jump-start or replacement.", 0.00},
                        {"Check Engine Light On", "Engine warning light illuminated.", 0.00},
                        {"Dim / Flickering Lights", "Headlights or interior lights dimming.", 0.00},
                        {"Power Windows Not Working", "Windows won't go up or down.", 0.00},
                        {"Central Locking Problem", "Doors won't lock/unlock remotely.", 0.00}
                    }
                },
                {
                    "Air Conditioning Problems",
                    {
                        {"AC Not Cooling", "Air conditioning blowing warm air.", 0.00},
                        {"AC Noisy", "Unusual noises when AC is on.", 0.00},
                        {"AC Smells Bad", "Unpleasant odor from AC vents.", 0.00},
                        {"AC Not Blowing Air", "No air coming from AC vents.", 0.00}
                    }
                },
                {
                    "Other Problems",
                    {
                        {"Exhaust Leak / Noisy", "Loud exhaust or smell of fumes.", 0.00},
                        {"Fuel System Problem", "Poor fuel economy or fuel smell.", 0.00},
                        {"Clutch Problem", "Difficulty shifting or clutch slipping.", 0.00},
                        {"Windscreen Crack / Chip", "Cracked or chipped windscreen.", 0.00},
                        {"General Vehicle Inspection", "Comprehensive vehicle check.", 0.00}
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
        std::cout << "                  SELECT PROBLEM(S) OR SERVICE(S)                      \n";
        std::cout << "========================================================================\n";

        if (!selectedItems.empty()) {
            std::cout << "CURRENTLY SELECTED (" << selectedItems.size() << " item(s)):\n";
            for (size_t i = 0; i < selectedItems.size(); ++i) {
                if (selectedItems[i].fee > 0) {
                    std::cout << "  " << (i + 1) << ". " << selectedItems[i].fullDescription
                        << " [Fee: $" << std::fixed << std::setprecision(2) << selectedItems[i].fee << "]\n";
                }
                else {
                    std::cout << "  " << (i + 1) << ". " << selectedItems[i].fullDescription << "\n";
                }
            }
            std::cout << "  TOTAL ESTIMATED SERVICE FEE: $" << std::fixed << std::setprecision(2) << totalServiceFee << "\n";
            std::cout << "------------------------------------------------------------------------\n";
        }

        std::cout << "1. Car Problems / Repair Issues\n";
        std::cout << "2. Scheduled Maintenance / Servicing\n";
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

                if (catChoice == 0) break;

                if (catChoice >= 1 && catChoice <= static_cast<int>(selectedMain.categories.size())) {
                    const auto& selectedCat = selectedMain.categories[catChoice - 1];

                    while (true) {
                        std::cout << "\n------------------------------------------------------------------------\n";
                        std::cout << "    SPECIFIC OPTIONS FOR: " << selectedCat.categoryName << "\n";
                        std::cout << "------------------------------------------------------------------------\n";
                        for (size_t s = 0; s < selectedCat.subOptions.size(); ++s) {
                            if (selectedCat.subOptions[s].fee > 0) {
                                std::cout << (s + 1) << ". " << selectedCat.subOptions[s].name
                                    << " [Fee: $" << std::fixed << std::setprecision(2) << selectedCat.subOptions[s].fee << "]\n"
                                    << "    Desc: " << selectedCat.subOptions[s].description << "\n";
                            }
                            else {
                                std::cout << (s + 1) << ". " << selectedCat.subOptions[s].name << "\n"
                                    << "    Desc: " << selectedCat.subOptions[s].description << "\n";
                            }
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

                        if (subChoice == 0) break;

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
                                if (sub.fee > 0) {
                                    std::cout << "\n>>> ADDED SUCCESSFULLY: " << itemStr << " ($" << std::fixed << std::setprecision(2) << sub.fee << ") <<<\n";
                                }
                                else {
                                    std::cout << "\n>>> ADDED SUCCESSFULLY: " << itemStr << " <<<\n";
                                }
                            }
                            break;
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
        if (selectedItems[i].fee > 0) {
            std::stringstream feeStream;
            feeStream << std::fixed << std::setprecision(2) << selectedItems[i].fee;
            combinedServices += "(" + std::to_string(i + 1) + ") " + selectedItems[i].fullDescription + " [$" + feeStream.str() + "]";
        }
        else {
            combinedServices += "(" + std::to_string(i + 1) + ") " + selectedItems[i].fullDescription;
        }
        if (i < selectedItems.size() - 1) {
            combinedServices += " | ";
        }
    }

    // Append total fixed fee summary if there are any services with fees
    if (totalServiceFee > 0) {
        std::stringstream totalStream;
        totalStream << std::fixed << std::setprecision(2) << totalServiceFee;
        combinedServices += " {Total Service Fee: $" + totalStream.str() + "}";
    }

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
    // NOTE: Booked services/problems are LABOR (scheduled work), not physical
    // parts, so both the fee AND the service name belong with laborCost /
    // lastServiceDescription. partsCost/lastPartsUsed stay untouched here -
    // those are only ever set later in service.cpp when a technician logs an
    // actual replaced part during the job.
    for (auto& v : targetCustomer->vehicles) {
        if (v.licensePlate == app.licensePlate) {
            v.laborCost += totalServiceFee;
            v.currentServiceCost += totalServiceFee;
            v.lastServiceDescription = combinedServices;
            v.status = "In Service Queue";
            break;
        }
    }
    saveCustomersToFile(customers);

    app.appointmentId = generateNextAppointmentId(appointments);
    app.totalFee = totalServiceFee;
    app.isActive = true;
    appointments.push_back(app);

    std::cout << "\nBooking successful! Vehicle service cost updated and passed to Billing.\n";
    generateBookingSlip(app);
}

void generateBookingSlip(const Appointment& app) {
    std::cout << "\n========================================\n";
    std::cout << "         APPOINTMENT CONFIRMATION       \n";
    std::cout << "========================================\n";
    std::cout << "Booking ID      : " << app.appointmentId << "\n";
    std::cout << "Customer Phone  : " << app.customerId << "\n";
    std::cout << "Vehicle Plate   : " << app.licensePlate << "\n";
    std::cout << "Schedule        : " << app.date << " @ " << app.time << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Selected Items  :\n";

    // Parse and display each item on a separate line
    std::string serviceType = app.serviceType;
    size_t pos = 0;
    int itemCount = 0;

    // Find each numbered item
    while (true) {
        size_t startPos = serviceType.find("(" + std::to_string(itemCount + 1) + ")", pos);
        if (startPos == std::string::npos) break;

        size_t endPos = serviceType.find(" | ", startPos);
        if (endPos == std::string::npos) {
            // Last item - check for {Total Service Fee} or end of string
            size_t totalPos = serviceType.find(" {Total Service Fee:", startPos);
            if (totalPos != std::string::npos) {
                endPos = totalPos;
            }
            else {
                endPos = serviceType.length();
            }
        }

        std::string item = serviceType.substr(startPos, endPos - startPos);
        // Remove the "(#) " prefix
        size_t prefixEnd = item.find(") ");
        if (prefixEnd != std::string::npos) {
            item = item.substr(prefixEnd + 2);
        }

        // Remove the category prefix (everything before " -> ")
        size_t arrowPos = item.find(" -> ");
        if (arrowPos != std::string::npos) {
            item = item.substr(arrowPos + 4); // Keep only the item name after " -> "
        }

        // Check if this item has a price
        size_t pricePos = item.find(" [$");
        if (pricePos != std::string::npos) {
            // Item has price - show it
            std::cout << "  " << (itemCount + 1) << ". " << item << "\n";
        }
        else {
            // Problem item - no price
            std::cout << "  " << (itemCount + 1) << ". " << item << "\n";
        }

        itemCount++;
        pos = endPos + 3; // Skip past " | "
        if (pos >= serviceType.length()) break;
    }

    // Extract and display total fee if present
    size_t totalPos = serviceType.find("{Total Service Fee: $");
    if (totalPos != std::string::npos) {
        size_t endTotal = serviceType.find("}", totalPos);
        if (endTotal != std::string::npos) {
            std::string totalFee = serviceType.substr(totalPos, endTotal - totalPos + 1);
            std::cout << "----------------------------------------\n";
            std::cout << "Total Fee       : " << totalFee << "\n";
        }
    }

    std::cout << "Status          : " << (app.isActive ? "ACTIVE" : "INACTIVE") << "\n";
    std::cout << "========================================\n";
}

void viewAppointmentsMenu(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
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
            viewAllAppointments(appointments, customers);
            break;
        case 2:
            viewTodaysAppointments(appointments, customers);
            break;
        case 3:
            viewAppointmentsByDate(appointments, customers);
            break;
        case 0:
            std::cout << "Returning to Appointment Menu...\n";
            break;
        default:
            std::cout << "Invalid choice!\n";
        }
    } while (choice != 0);
}

void viewAllAppointments(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
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

    manageAppointmentFromList(foundAppointments, appointments, customers);
}

void viewAppointmentsByDate(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
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

    manageAppointmentFromList(foundAppointments, appointments, customers);
}

void viewTodaysAppointments(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
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

    manageAppointmentFromList(foundAppointments, appointments, customers);
}

static void manageAppointmentFromList(std::vector<Appointment*>& foundAppointments,
    std::vector<Appointment>& allAppointments, std::vector<Customer>& customers) {
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
            cancelSpecificAppointment(selectedApp, customers);
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

static void cancelSpecificAppointment(Appointment* app, std::vector<Customer>& customers) {
    if (!app) {
        std::cout << "Appointment not found.\n";
        return;
    }

    if (!app->isActive) {
        std::cout << "This appointment is already inactive.\n";
        return;
    }

    if (app->totalFee > 0.0) {
        for (auto& c : customers) {
            for (auto& v : c.vehicles) {
                if (v.licensePlate == app->licensePlate) {
                    v.laborCost -= app->totalFee;
                    v.currentServiceCost -= app->totalFee;
                    if (v.laborCost < 0.0) v.laborCost = 0.0;
                    if (v.currentServiceCost < 0.0) v.currentServiceCost = 0.0;
                    v.lastServiceDescription.clear(); // belonged solely to this (now cancelled) appointment
                    break;
                }
            }
        }
        saveCustomersToFile(customers);
    }

    app->isActive = false;
    std::cout << "Appointment " << app->appointmentId << " successfully set to inactive.\n";
}

void searchAppointment(const std::vector<Appointment>& appointments) {
    std::string query;
    std::cout << "\nEnter Booking ID or Customer Phone Number (or 0 to Return to Main Menu): ";
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

void cancelAppointment(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
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
            cancelSpecificAppointment(&a, customers);
            saveAppointmentsToFile(appointments);
            return;
        }
    }
    std::cout << "Appointment ID not found.\n";
}