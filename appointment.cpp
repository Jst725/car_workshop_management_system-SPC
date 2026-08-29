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
#include <cstdlib>

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

// Function to clear the console screen cross-platform
static void clearScreen() {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

// Function to pause program execution and prompt for Enter key
static void pause() {
    std::cout << Color::BRIGHT_YELLOW << "\nPress Enter to continue..." << Color::RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

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
    std::cout << Color::BRIGHT_WHITE << std::left << std::setw(10) << a.appointmentId
        << std::setw(12) << a.customerId
        << std::setw(12) << a.licensePlate
        << std::setw(14) << a.date
        << std::setw(10) << a.time
        << (a.isActive ? (Color::BRIGHT_CYAN + "Active" + Color::RESET) : (Color::BRIGHT_RED + "Inactive" + Color::RESET)) << "\n";
}

static void displayAppointmentHeader() {
    std::cout << Color::CYAN << "\n----------------------------------------------------------------------------------------\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << std::left << std::setw(5) << "No."
        << std::setw(10) << "Appt ID"
        << std::setw(12) << "Phone"
        << std::setw(12) << "Plate"
        << std::setw(14) << "Date"
        << std::setw(10) << "Time"
        << "Status\n" << Color::RESET;
    std::cout << Color::CYAN << "----------------------------------------------------------------------------------------\n" << Color::RESET;
}

void appointmentMenu(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
    int choice = -1;
    do {
        clearScreen();
        std::cout << Color::CYAN << "\n========================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "         APPOINTMENT MANAGEMENT         \n" << Color::RESET;
        std::cout << Color::CYAN << "========================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Book Appointment\n";
        std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "View Appointments\n";
        std::cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Search Appointment\n";
        std::cout << Color::BRIGHT_CYAN << "4. " << Color::BRIGHT_WHITE << "Reschedule Appointment\n";
        std::cout << Color::BRIGHT_CYAN << "5. " << Color::BRIGHT_WHITE << "Cancel Appointment\n";
        std::cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Return to Main Menu\n";
        std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;

        if (!(std::cin >> choice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            pause();
            continue;
        }

        switch (choice) {
        case 1:
            clearScreen();
            bookAppointment(appointments, customers);
            saveAppointmentsToFile(appointments);
            pause();
            break;
        case 2:
            clearScreen();
            viewAppointmentsMenu(appointments, customers);
            break;
        case 3:
            clearScreen();
            searchAppointment(appointments);
            pause();
            break;
        case 4:
            clearScreen();
            rescheduleAppointment(appointments);
            saveAppointmentsToFile(appointments);
            pause();
            break;
        case 5:
            clearScreen();
            cancelAppointment(appointments, customers);
            saveAppointmentsToFile(appointments);
            pause();
            break;
        case 0:
            std::cout << Color::BRIGHT_CYAN << "Returning to Main Menu...\n" << Color::RESET;
            break;
        default:
            std::cout << Color::BRIGHT_RED << "Invalid choice! Please select an option between 0 and 5.\n" << Color::RESET;
            pause();
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

    std::cout << Color::BRIGHT_YELLOW << "\nEnter Customer Phone Number (or 0 to Return to Main Menu): " << Color::RESET;
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
        std::cout << Color::BRIGHT_RED << "ERROR: Customer phone number not found!\n" << Color::RESET;
        return;
    }

    std::cout << Color::BRIGHT_CYAN << "\nCustomer Found: " << Color::BRIGHT_WHITE << targetCustomer->name << " (" << targetCustomer->phone << ")\n" << Color::RESET;

    std::string selectedPlate;
    while (true) {
        std::cout << Color::CYAN << "\n----------------------------------------\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "SELECT VEHICLE FOR APPOINTMENT:\n" << Color::RESET;
        std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;

        int optionNumber = 1;
        for (const auto& v : targetCustomer->vehicles) {
            std::cout << Color::BRIGHT_CYAN << optionNumber << ". " << Color::BRIGHT_WHITE << v.licensePlate
                << " (" << v.brand << " " << v.model << ")\n";
            optionNumber++;
        }
        std::cout << Color::BRIGHT_CYAN << optionNumber << ". " << Color::BRIGHT_YELLOW << "[+ Add New Vehicle]\n" << Color::RESET;
        std::cout << Color::BRIGHT_RED << "0. Cancel Appointment\n" << Color::RESET;
        std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << "Choice: " << Color::RESET;

        int vChoice;
        if (!(std::cin >> vChoice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input! Enter a valid menu option number.\n" << Color::RESET;
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        if (vChoice == 0) {
            std::cout << Color::BRIGHT_YELLOW << "Appointment booking cancelled.\n" << Color::RESET;
            return;
        }

        if (vChoice >= 1 && vChoice <= static_cast<int>(targetCustomer->vehicles.size())) {
            selectedPlate = targetCustomer->vehicles[vChoice - 1].licensePlate;
            break;
        }
        else if (vChoice == optionNumber) {
            Vehicle newV;

            while (true) {
                std::cout << Color::BRIGHT_YELLOW << "\nEnter New Vehicle License Plate: " << Color::RESET;
                std::cin >> newV.licensePlate;
                newV.licensePlate = toUpper(newV.licensePlate);

                if (!isValidLicensePlate(newV.licensePlate)) {
                    std::cout << Color::BRIGHT_RED << "ERROR: License plate must contain at least 1 letter, at least 1 digit, and no spaces/special characters!\n" << Color::RESET;
                }
                else if (isPlateTaken(customers, newV.licensePlate)) {
                    std::cout << Color::BRIGHT_RED << "ERROR: This license plate is already registered to another customer!\n" << Color::RESET;
                }
                else {
                    break;
                }
            }

            std::cin.ignore(1000, '\n');
            std::cout << Color::BRIGHT_YELLOW << "Enter Vehicle Brand (e.g. Honda): " << Color::RESET;
            std::getline(std::cin, newV.brand);
            newV.brand = toUpper(newV.brand);

            std::cout << Color::BRIGHT_YELLOW << "Enter Vehicle Model (e.g. Civic): " << Color::RESET;
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
            std::cout << Color::BRIGHT_CYAN << "\nVehicle [" << selectedPlate << "] registered and selected successfully!\n" << Color::RESET;
            break;
        }
        else {
            std::cout << Color::BRIGHT_RED << "Invalid selection! Please select a number from the menu.\n" << Color::RESET;
        }
    }

    app.licensePlate = toUpper(selectedPlate);

    // --- ENFORCE: only ONE active appointment allowed per vehicle at a time ---
    for (const auto& a : appointments) {
        if (a.isActive && a.licensePlate == app.licensePlate) {
            std::cout << Color::BRIGHT_RED << "\nERROR: This vehicle [" << app.licensePlate << "] already has an active appointment ("
                << a.appointmentId << ", " << a.date << " @ " << a.time << ").\n" << Color::RESET;
            std::cout << Color::BRIGHT_YELLOW << "Please reschedule/cancel the existing appointment before booking a new one.\n" << Color::RESET;
            return;
        }
    }

    // 1. SELECT DATE
    std::string today = getTodayDate();
    while (true) {
        std::cout << Color::BRIGHT_YELLOW << "\nEnter Service Date (YYYY-MM-DD, Today or Future): " << Color::RESET;
        std::cin >> app.date;
        if (!isValidDate(app.date)) {
            std::cout << Color::BRIGHT_RED << "ERROR: Invalid date format or calendar day! Use YYYY-MM-DD.\n" << Color::RESET;
        }
        else if (app.date < today) {
            std::cout << Color::BRIGHT_RED << "ERROR: Appointment date cannot be in the past! Current date is: " << today << "\n" << Color::RESET;
        }
        else {
            break;
        }
    }

    // 2. SELECT TIME SLOT FROM MENU (09:00 to 22:00, hiding full slots)
    const std::vector<std::string> allSlots = {
        "09:00", "10:00", "11:00", "12:00", "13:00", "14:00", "15:00",
        "16:00", "17:00", "18:00", "19:00", "20:00", "21:00", "22:00"
    };
    const int MAX_CARS_PER_SLOT = 3;

    while (true) {
        std::vector<std::string> availableSlots;
        std::vector<int> spotsRemainingList;

        for (const auto& slot : allSlots) {
            int bookedCount = 0;
            for (const auto& a : appointments) {
                if (a.isActive && a.date == app.date && a.time == slot) {
                    bookedCount++;
                }
            }

            int spotsLeft = MAX_CARS_PER_SLOT - bookedCount;
            if (spotsLeft > 0) {
                availableSlots.push_back(slot);
                spotsRemainingList.push_back(spotsLeft);
            }
        }

        if (availableSlots.empty()) {
            std::cout << Color::BRIGHT_RED << "\nERROR: All time slots are fully booked for this date! Please choose another date.\n" << Color::RESET;
            while (true) {
                std::cout << Color::BRIGHT_YELLOW << "\nEnter New Service Date (YYYY-MM-DD, Today or Future): " << Color::RESET;
                std::cin >> app.date;
                if (!isValidDate(app.date)) {
                    std::cout << Color::BRIGHT_RED << "ERROR: Invalid date format or calendar day! Use YYYY-MM-DD.\n" << Color::RESET;
                }
                else if (app.date < today) {
                    std::cout << Color::BRIGHT_RED << "ERROR: Appointment date cannot be in the past!\n" << Color::RESET;
                }
                else {
                    break;
                }
            }
            continue;
        }

        std::cout << Color::CYAN << "\n----------------------------------------\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "SELECT AVAILABLE TIME SLOT:\n" << Color::RESET;
        std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;

        for (size_t i = 0; i < availableSlots.size(); ++i) {
            std::cout << Color::BRIGHT_CYAN << (i + 1) << ". " << Color::BRIGHT_WHITE << availableSlots[i]
                << Color::LIGHT_GRAY << " (" << spotsRemainingList[i] << " spot(s) remaining)\n" << Color::RESET;
        }
        std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << "Choice (1-" << availableSlots.size() << ") [Enter 0 to change date]: " << Color::RESET;

        int slotChoice;
        if (!(std::cin >> slotChoice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        if (slotChoice == 0) {
            while (true) {
                std::cout << Color::BRIGHT_YELLOW << "\nEnter Service Date (YYYY-MM-DD, Today or Future): " << Color::RESET;
                std::cin >> app.date;
                if (!isValidDate(app.date)) {
                    std::cout << Color::BRIGHT_RED << "ERROR: Invalid date format or calendar day! Use YYYY-MM-DD.\n" << Color::RESET;
                }
                else if (app.date < today) {
                    std::cout << Color::BRIGHT_RED << "ERROR: Appointment date cannot be in the past!\n" << Color::RESET;
                }
                else {
                    break;
                }
            }
            continue;
        }

        if (slotChoice >= 1 && slotChoice <= static_cast<int>(availableSlots.size())) {
            std::string selectedTime = availableSlots[slotChoice - 1];

            int currentSlotCount = 0;
            for (const auto& a : appointments) {
                if (a.isActive && a.date == app.date && a.time == selectedTime) {
                    currentSlotCount++;
                }
            }

            if (currentSlotCount >= MAX_CARS_PER_SLOT) {
                std::cout << Color::BRIGHT_RED << "ERROR: This slot is fully booked! Please pick another time.\n" << Color::RESET;
            }
            else {
                app.time = selectedTime;
                break;
            }
        }
        else {
            std::cout << Color::BRIGHT_RED << "Invalid choice! Please select a valid slot number.\n" << Color::RESET;
        }
    }

    // --- TWO-TIERED BOOKING CATALOG ---
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
        std::cout << Color::CYAN << "\n========================================================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "                  SELECT PROBLEM(S) OR SERVICE(S)                      \n" << Color::RESET;
        std::cout << Color::CYAN << "========================================================================\n" << Color::RESET;

        if (!selectedItems.empty()) {
            std::cout << Color::BRIGHT_CYAN << "CURRENTLY SELECTED (" << selectedItems.size() << " item(s)):\n" << Color::RESET;
            for (size_t i = 0; i < selectedItems.size(); ++i) {
                std::string shortDescription = selectedItems[i].fullDescription;
                size_t arrowPos = shortDescription.find(" -> ");
                if (arrowPos != std::string::npos) {
                    shortDescription = shortDescription.substr(arrowPos + 4);
                }

                if (selectedItems[i].fee > 0) {
                    std::cout << "  " << (i + 1) << ". " << Color::BRIGHT_WHITE << shortDescription
                        << Color::BRIGHT_YELLOW << " [Fee: RM" << std::fixed << std::setprecision(2) << selectedItems[i].fee << "]\n" << Color::RESET;
                }
                else {
                    std::cout << "  " << (i + 1) << ". " << Color::BRIGHT_WHITE << shortDescription << "\n" << Color::RESET;
                }
            }
            std::cout << Color::BRIGHT_YELLOW << "  TOTAL ESTIMATED SERVICE FEE: RM" << std::fixed << std::setprecision(2) << totalServiceFee << "\n" << Color::RESET;
            std::cout << Color::CYAN << "------------------------------------------------------------------------\n" << Color::RESET;
        }

        std::cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Car Problems / Repair Issues\n";
        std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Scheduled Maintenance / Servicing\n";
        if (!selectedItems.empty()) {
            std::cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_YELLOW << "[DONE] Finish Selection and Proceed (RM" << std::fixed << std::setprecision(2) << totalServiceFee << " total)\n" << Color::RESET;
        }
        std::cout << Color::BRIGHT_RED << "0. Cancel Booking\n" << Color::RESET;
        std::cout << Color::CYAN << "------------------------------------------------------------------------\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << "Select Option: " << Color::RESET;

        int mainChoice;
        if (!(std::cin >> mainChoice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        if (mainChoice == 0) {
            std::cout << Color::BRIGHT_YELLOW << "Appointment booking cancelled.\n" << Color::RESET;
            return;
        }

        if (mainChoice == 3 && !selectedItems.empty()) {
            doneSelecting = true;
            break;
        }

        if (mainChoice == 1 || mainChoice == 2) {
            const auto& selectedMain = mainTypes[mainChoice - 1];

            while (true) {
                std::cout << Color::CYAN << "\n========================================================================\n" << Color::RESET;
                std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "    CATEGORIES FOR: " << selectedMain.typeName << "\n" << Color::RESET;
                std::cout << Color::CYAN << "========================================================================\n" << Color::RESET;
                for (size_t c = 0; c < selectedMain.categories.size(); ++c) {
                    std::cout << Color::BRIGHT_CYAN << (c + 1) << ". " << Color::BRIGHT_WHITE << selectedMain.categories[c].categoryName << "\n";
                }
                std::cout << Color::BRIGHT_RED << "0. Back to Main Selection\n" << Color::RESET;
                std::cout << Color::CYAN << "------------------------------------------------------------------------\n" << Color::RESET;
                std::cout << Color::BRIGHT_YELLOW << "Select Category (1 - " << selectedMain.categories.size() << "): " << Color::RESET;

                int catChoice;
                if (!(std::cin >> catChoice)) {
                    std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
                    std::cin.clear();
                    std::cin.ignore(1000, '\n');
                    continue;
                }

                if (catChoice == 0) break;

                if (catChoice >= 1 && catChoice <= static_cast<int>(selectedMain.categories.size())) {
                    const auto& selectedCat = selectedMain.categories[catChoice - 1];

                    while (true) {
                        std::cout << Color::CYAN << "\n------------------------------------------------------------------------\n" << Color::RESET;
                        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "    SPECIFIC OPTIONS FOR: " << selectedCat.categoryName << "\n" << Color::RESET;
                        std::cout << Color::CYAN << "------------------------------------------------------------------------\n" << Color::RESET;
                        for (size_t s = 0; s < selectedCat.subOptions.size(); ++s) {
                            if (selectedCat.subOptions[s].fee > 0) {
                                std::cout << Color::BRIGHT_CYAN << (s + 1) << ". " << Color::BRIGHT_WHITE << selectedCat.subOptions[s].name
                                    << Color::BRIGHT_YELLOW << " [Fee: RM" << std::fixed << std::setprecision(2) << selectedCat.subOptions[s].fee << "]\n"
                                    << Color::LIGHT_GRAY << "    Desc: " << selectedCat.subOptions[s].description << "\n";
                            }
                            else {
                                std::cout << Color::BRIGHT_CYAN << (s + 1) << ". " << Color::BRIGHT_WHITE << selectedCat.subOptions[s].name << "\n"
                                    << Color::LIGHT_GRAY << "    Desc: " << selectedCat.subOptions[s].description << "\n";
                            }
                            std::cout << Color::CYAN << "------------------------------------------------------------------------\n" << Color::RESET;
                        }
                        std::cout << Color::BRIGHT_RED << "0. Back to Categories\n" << Color::RESET;
                        std::cout << Color::BRIGHT_YELLOW << "Select Specific Issue / Service to ADD: " << Color::RESET;

                        int subChoice;
                        if (!(std::cin >> subChoice)) {
                            std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
                            std::cin.clear();
                            std::cin.ignore(1000, '\n');
                            continue;
                        }

                        if (subChoice == 0) break;

                        if (subChoice >= 1 && subChoice <= static_cast<int>(selectedCat.subOptions.size())) {
                            const auto& sub = selectedCat.subOptions[subChoice - 1];
                            std::string itemStr = "[" + selectedMain.typeName + "] " + selectedCat.categoryName + " -> " + sub.name;

                            bool exists = false;
                            for (const auto& item : selectedItems) {
                                if (item.fullDescription == itemStr) {
                                    exists = true;
                                    break;
                                }
                            }

                            if (exists) {
                                std::cout << Color::BRIGHT_RED << "\n>>> NOTE: This item is already in your selection list! <<<\n" << Color::RESET;
                            }
                            else {
                                selectedItems.push_back({ itemStr, sub.fee });
                                totalServiceFee += sub.fee;
                                if (sub.fee > 0) {
                                    std::cout << Color::BRIGHT_CYAN << "\n>>> ADDED SUCCESSFULLY: " << itemStr << " (RM" << std::fixed << std::setprecision(2) << sub.fee << ") <<<\n" << Color::RESET;
                                }
                                else {
                                    std::cout << Color::BRIGHT_CYAN << "\n>>> ADDED SUCCESSFULLY: " << itemStr << " <<<\n" << Color::RESET;
                                }
                            }
                            break;
                        }
                        else {
                            std::cout << Color::BRIGHT_RED << "Invalid selection! Try again.\n" << Color::RESET;
                        }
                    }
                }
                else {
                    std::cout << Color::BRIGHT_RED << "Invalid category selection! Try again.\n" << Color::RESET;
                }
            }
        }
        else {
            std::cout << Color::BRIGHT_RED << "Invalid selection! Please choose a valid option.\n" << Color::RESET;
        }
    }

    std::string combinedServices = "";
    for (size_t i = 0; i < selectedItems.size(); ++i) {
        std::string shortDescription = selectedItems[i].fullDescription;
        size_t arrowPos = shortDescription.find(" -> ");
        if (arrowPos != std::string::npos) {
            shortDescription = shortDescription.substr(arrowPos + 4);
        }

        if (selectedItems[i].fee > 0) {
            std::stringstream feeStream;
            feeStream << std::fixed << std::setprecision(2) << selectedItems[i].fee;
            combinedServices += "(" + std::to_string(i + 1) + ") " + shortDescription + " [RM" + feeStream.str() + "]";
        }
        else {
            combinedServices += "(" + std::to_string(i + 1) + ") " + shortDescription;
        }
        if (i < selectedItems.size() - 1) {
            combinedServices += " | ";
        }
    }

    if (totalServiceFee > 0) {
        std::stringstream totalStream;
        totalStream << std::fixed << std::setprecision(2) << totalServiceFee;
        combinedServices += " {Total Service Fee: RM" + totalStream.str() + "}";
    }

    // --- OPTIONAL REMARK / NOTE INPUT ---
    std::cin.ignore(1000, '\n');
    std::string remarkInput = "";
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Additional Remarks / Symptoms Description (Press Enter to skip): " << Color::RESET;
    std::getline(std::cin, remarkInput);

    if (!remarkInput.empty()) {
        app.serviceType = combinedServices + " [Remark: " + remarkInput + "]";
    }
    else {
        app.serviceType = combinedServices;
    }

    for (auto& v : targetCustomer->vehicles) {
        if (v.licensePlate == app.licensePlate) {
            v.partsCost += totalServiceFee;
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

    std::cout << Color::BRIGHT_CYAN << "\nBooking successful! Vehicle service cost updated and passed to Billing.\n" << Color::RESET;
    generateBookingSlip(app);
}

void generateBookingSlip(const Appointment& app) {
    std::cout << Color::CYAN << "\n========================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "         APPOINTMENT CONFIRMATION       \n" << Color::RESET;
    std::cout << Color::CYAN << "========================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << "Booking ID      : " << Color::BRIGHT_WHITE << app.appointmentId << "\n";
    std::cout << Color::BRIGHT_CYAN << "Customer Phone  : " << Color::BRIGHT_WHITE << app.customerId << "\n";
    std::cout << Color::BRIGHT_CYAN << "Vehicle Plate   : " << Color::BRIGHT_WHITE << app.licensePlate << "\n";
    std::cout << Color::BRIGHT_CYAN << "Schedule        : " << Color::BRIGHT_WHITE << app.date << " @ " << app.time << "\n";
    std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << "Selected Items  :\n" << Color::RESET;

    std::string serviceType = app.serviceType;
    size_t pos = 0;
    int itemCount = 0;

    while (true) {
        size_t startPos = serviceType.find("(" + std::to_string(itemCount + 1) + ")", pos);
        if (startPos == std::string::npos) break;

        size_t endPos = serviceType.find(" | ", startPos);
        if (endPos == std::string::npos) {
            size_t totalPos = serviceType.find(" {Total Service Fee:", startPos);
            if (totalPos != std::string::npos) {
                endPos = totalPos;
            }
            else {
                endPos = serviceType.length();
            }
        }

        std::string item = serviceType.substr(startPos, endPos - startPos);
        size_t prefixEnd = item.find(") ");
        if (prefixEnd != std::string::npos) {
            item = item.substr(prefixEnd + 2);
        }

        size_t arrowPos = item.find(" -> ");
        if (arrowPos != std::string::npos) {
            item = item.substr(arrowPos + 4);
        }

        std::cout << "  " << (itemCount + 1) << ". " << Color::BRIGHT_WHITE << item << "\n";

        itemCount++;
        pos = endPos + 3;
        if (pos >= serviceType.length()) break;
    }

    size_t totalPos = serviceType.find("{Total Service Fee: RM");
    if (totalPos != std::string::npos) {
        size_t endTotal = serviceType.find("}", totalPos);
        if (endTotal != std::string::npos) {
            std::string totalFee = serviceType.substr(totalPos, endTotal - totalPos + 1);
            std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
            std::cout << Color::BRIGHT_YELLOW << "Total Fee       : " << totalFee << "\n" << Color::RESET;
        }
    }

    std::cout << Color::BRIGHT_CYAN << "Status          : " << (app.isActive ? (Color::BRIGHT_CYAN + "ACTIVE" + Color::RESET) : (Color::BRIGHT_RED + "INACTIVE" + Color::RESET)) << "\n";
    std::cout << Color::CYAN << "========================================\n" << Color::RESET;
}

void viewAppointmentsMenu(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
    int choice = -1;
    do {
        clearScreen();
        std::cout << Color::CYAN << "\n========================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "            VIEW APPOINTMENTS           \n" << Color::RESET;
        std::cout << Color::CYAN << "========================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "View All Appointments (Sorted by Appt ID)\n";
        std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "View Today's Appointments (Sorted by Time)\n";
        std::cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "View Appointments by Date (Sorted by Time)\n";
        std::cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Return to Appointment Menu\n";
        std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << "Enter choice: " << Color::RESET;

        if (!(std::cin >> choice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input!\n" << Color::RESET;
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            pause();
            continue;
        }

        switch (choice) {
        case 1:
            clearScreen();
            viewAllAppointments(appointments, customers);
            pause();
            break;
        case 2:
            clearScreen();
            viewTodaysAppointments(appointments, customers);
            pause();
            break;
        case 3:
            clearScreen();
            viewAppointmentsByDate(appointments, customers);
            pause();
            break;
        case 0:
            std::cout << Color::BRIGHT_CYAN << "Returning to Appointment Menu...\n" << Color::RESET;
            break;
        default:
            std::cout << Color::BRIGHT_RED << "Invalid choice!\n" << Color::RESET;
            pause();
        }
    } while (choice != 0);
}

void viewAllAppointments(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
    if (appointments.empty()) {
        std::cout << Color::BRIGHT_YELLOW << "\nNo appointments found.\n" << Color::RESET;
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
        std::cout << Color::BRIGHT_CYAN << std::left << std::setw(5) << (std::to_string(index++) + ".");
        displayAppointmentRow(*a);
    }

    manageAppointmentFromList(foundAppointments, appointments, customers);
}

void viewAppointmentsByDate(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
    std::string date;
    while (true) {
        std::cout << Color::BRIGHT_YELLOW << "\nEnter Date to view (YYYY-MM-DD, or 0 to Return): " << Color::RESET;
        std::cin >> date;
        if (date == "0") return;

        if (!isValidDate(date)) {
            std::cout << Color::BRIGHT_RED << "ERROR: Invalid date format! Use YYYY-MM-DD.\n" << Color::RESET;
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
        std::cout << Color::BRIGHT_YELLOW << "No appointments found for date: " << date << "\n" << Color::RESET;
        return;
    }

    std::sort(foundAppointments.begin(), foundAppointments.end(), [](const Appointment* a, const Appointment* b) {
        return a->time < b->time;
        });

    displayAppointmentHeader();
    int index = 1;
    for (auto* a : foundAppointments) {
        std::cout << Color::BRIGHT_CYAN << std::left << std::setw(5) << (std::to_string(index++) + ".");
        displayAppointmentRow(*a);
    }

    manageAppointmentFromList(foundAppointments, appointments, customers);
}

void viewTodaysAppointments(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
    std::string today = getTodayDate();
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "\n=== TODAY'S APPOINTMENTS (" << today << ") ===\n" << Color::RESET;

    std::vector<Appointment*> foundAppointments;
    for (auto& a : appointments) {
        if (a.date == today) {
            foundAppointments.push_back(&a);
        }
    }

    if (foundAppointments.empty()) {
        std::cout << Color::BRIGHT_YELLOW << "No appointments scheduled for today.\n" << Color::RESET;
        return;
    }

    std::sort(foundAppointments.begin(), foundAppointments.end(), [](const Appointment* a, const Appointment* b) {
        return a->time < b->time;
        });

    displayAppointmentHeader();
    int index = 1;
    for (auto* a : foundAppointments) {
        std::cout << Color::BRIGHT_CYAN << std::left << std::setw(5) << (std::to_string(index++) + ".");
        displayAppointmentRow(*a);
    }

    manageAppointmentFromList(foundAppointments, appointments, customers);
}

static void manageAppointmentFromList(std::vector<Appointment*>& foundAppointments,
    std::vector<Appointment>& allAppointments, std::vector<Customer>& customers) {
    if (foundAppointments.empty()) return;

    std::cout << Color::BRIGHT_YELLOW << "\nDo you want to manage an appointment from this list?\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Reschedule an appointment\n";
    std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Cancel an appointment\n";
    std::cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Return to menu\n";
    std::cout << Color::BRIGHT_YELLOW << "Choice: " << Color::RESET;

    int choice;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        return;
    }

    if (choice == 0) return;

    if (choice == 1 || choice == 2) {
        int appointmentIndex;
        std::cout << Color::BRIGHT_YELLOW << "Enter the number of the appointment to "
            << (choice == 1 ? "reschedule" : "cancel")
            << " (1-" << foundAppointments.size() << "): " << Color::RESET;

        if (!(std::cin >> appointmentIndex) || appointmentIndex < 1 ||
            appointmentIndex > static_cast<int>(foundAppointments.size())) {
            std::cout << Color::BRIGHT_RED << "Invalid selection!\n" << Color::RESET;
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
        std::cout << Color::BRIGHT_RED << "Invalid choice!\n" << Color::RESET;
    }
}

static void rescheduleSpecificAppointment(Appointment* app, std::vector<Appointment>& allAppointments) {
    if (!app || !app->isActive) {
        std::cout << Color::BRIGHT_RED << "This appointment is not active or does not exist.\n" << Color::RESET;
        return;
    }

    std::cout << Color::BRIGHT_CYAN << "\nCurrent Schedule: " << Color::BRIGHT_WHITE << app->date << " @ " << app->time << "\n" << Color::RESET;

    std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Change Date Only\n";
    std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Change Time Slot Only\n";
    std::cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Change Both Date and Time Slot\n";
    std::cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Cancel Rescheduling\n";
    std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << "Select Option: " << Color::RESET;

    int choice;
    if (!(std::cin >> choice)) {
        std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        return;
    }

    if (choice == 0) return;

    std::string newDate = app->date;
    std::string newTime = app->time;
    std::string today = getTodayDate();

    // Change Date (Option 1 or 3)
    if (choice == 1 || choice == 3) {
        while (true) {
            std::cout << Color::BRIGHT_YELLOW << "\nEnter New Date (YYYY-MM-DD, Today or Future): " << Color::RESET;
            std::cin >> newDate;
            if (!isValidDate(newDate)) {
                std::cout << Color::BRIGHT_RED << "ERROR: Invalid date format or calendar day! Use YYYY-MM-DD.\n" << Color::RESET;
            }
            else if (newDate < today) {
                std::cout << Color::BRIGHT_RED << "ERROR: Rescheduled date cannot be in the past! Current date is: " << today << "\n" << Color::RESET;
            }
            else {
                break;
            }
        }
    }

    // Change Time Slot (Option 2 or 3) - 09:00 to 22:00, hiding full slots
    if (choice == 2 || choice == 3) {
        const std::vector<std::string> allSlots = {
            "09:00", "10:00", "11:00", "12:00", "13:00", "14:00", "15:00",
            "16:00", "17:00", "18:00", "19:00", "20:00", "21:00", "22:00"
        };
        const int MAX_CARS_PER_SLOT = 3;

        while (true) {
            std::vector<std::string> availableSlots;
            std::vector<int> spotsRemainingList;

            for (const auto& slot : allSlots) {
                int bookedCount = 0;
                for (const auto& a : allAppointments) {
                    if (&a != app && a.isActive && a.date == newDate && a.time == slot) {
                        bookedCount++;
                    }
                }

                int spotsLeft = MAX_CARS_PER_SLOT - bookedCount;
                if (spotsLeft > 0) {
                    availableSlots.push_back(slot);
                    spotsRemainingList.push_back(spotsLeft);
                }
            }

            if (availableSlots.empty()) {
                std::cout << Color::BRIGHT_RED << "\nERROR: All time slots are fully booked for this date! Please choose another date.\n" << Color::RESET;
                while (true) {
                    std::cout << Color::BRIGHT_YELLOW << "\nEnter New Date (YYYY-MM-DD, Today or Future): " << Color::RESET;
                    std::cin >> newDate;
                    if (!isValidDate(newDate)) {
                        std::cout << Color::BRIGHT_RED << "ERROR: Invalid date format!\n" << Color::RESET;
                    }
                    else if (newDate < today) {
                        std::cout << Color::BRIGHT_RED << "ERROR: Date cannot be in the past!\n" << Color::RESET;
                    }
                    else {
                        break;
                    }
                }
                continue;
            }

            std::cout << Color::CYAN << "\n----------------------------------------\n" << Color::RESET;
            std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "SELECT NEW TIME SLOT:\n" << Color::RESET;
            std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;

            for (size_t i = 0; i < availableSlots.size(); ++i) {
                std::cout << Color::BRIGHT_CYAN << (i + 1) << ". " << Color::BRIGHT_WHITE << availableSlots[i]
                    << Color::LIGHT_GRAY << " (" << spotsRemainingList[i] << " spot(s) remaining)\n" << Color::RESET;
            }
            std::cout << Color::CYAN << "----------------------------------------\n" << Color::RESET;
            std::cout << Color::BRIGHT_YELLOW << "Choice (1-" << availableSlots.size() << ") [Enter 0 to change date]: " << Color::RESET;

            int slotChoice;
            if (!(std::cin >> slotChoice)) {
                std::cout << Color::BRIGHT_RED << "Invalid input!\n" << Color::RESET;
                std::cin.clear();
                std::cin.ignore(1000, '\n');
                continue;
            }

            if (slotChoice == 0) {
                while (true) {
                    std::cout << Color::BRIGHT_YELLOW << "\nEnter New Date (YYYY-MM-DD, Today or Future): " << Color::RESET;
                    std::cin >> newDate;
                    if (!isValidDate(newDate)) {
                        std::cout << Color::BRIGHT_RED << "ERROR: Invalid date format!\n" << Color::RESET;
                    }
                    else if (newDate < today) {
                        std::cout << Color::BRIGHT_RED << "ERROR: Date cannot be in the past!\n" << Color::RESET;
                    }
                    else {
                        break;
                    }
                }
                continue;
            }

            if (slotChoice >= 1 && slotChoice <= static_cast<int>(availableSlots.size())) {
                std::string selectedTime = availableSlots[slotChoice - 1];

                int targetSlotCount = 0;
                for (const auto& a : allAppointments) {
                    if (&a != app && a.isActive && a.date == newDate && a.time == selectedTime) {
                        targetSlotCount++;
                    }
                }

                if (targetSlotCount >= MAX_CARS_PER_SLOT) {
                    std::cout << Color::BRIGHT_RED << "ERROR: This slot is fully booked! Refreshing list...\n" << Color::RESET;
                }
                else {
                    newTime = selectedTime;
                    break;
                }
            }
            else {
                std::cout << Color::BRIGHT_RED << "Invalid option! Try again.\n" << Color::RESET;
            }
        }
    }

    if (choice < 1 || choice > 3) {
        std::cout << Color::BRIGHT_RED << "Invalid selection! Rescheduling cancelled.\n" << Color::RESET;
        return;
    }

    std::string oldDate = app->date;
    std::string oldTime = app->time;
    app->date = newDate;
    app->time = newTime;

    std::cout << Color::BRIGHT_CYAN << "\nAppointment " << app->appointmentId
        << " rescheduled from " << oldDate << " @ " << oldTime
        << " to " << newDate << " @ " << newTime << " successfully!\n" << Color::RESET;

    saveAppointmentsToFile(allAppointments);
}

static void cancelSpecificAppointment(Appointment* app, std::vector<Customer>& customers) {
    if (!app) {
        std::cout << Color::BRIGHT_RED << "Appointment not found.\n" << Color::RESET;
        return;
    }

    if (!app->isActive) {
        std::cout << Color::BRIGHT_YELLOW << "This appointment is already inactive.\n" << Color::RESET;
        return;
    }

    if (app->totalFee > 0.0) {
        for (auto& c : customers) {
            for (auto& v : c.vehicles) {
                if (v.licensePlate == app->licensePlate) {
                    v.partsCost -= app->totalFee;
                    v.currentServiceCost -= app->totalFee;
                    if (v.partsCost < 0.0) v.partsCost = 0.0;
                    if (v.currentServiceCost < 0.0) v.currentServiceCost = 0.0;
                    v.lastServiceDescription.clear();
                    break;
                }
            }
        }
        saveCustomersToFile(customers);
    }

    app->isActive = false;
    std::cout << Color::BRIGHT_CYAN << "Appointment " << app->appointmentId << " successfully set to inactive.\n" << Color::RESET;
}

void searchAppointment(const std::vector<Appointment>& appointments) {
    std::string query;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter Booking ID, Customer Phone Number, or License Plate (or 0 to Return to Main Menu): " << Color::RESET;
    std::cin >> query;
    if (query == "0") return;

    query = toUpper(query);

    std::vector<const Appointment*> matches;
    for (const auto& a : appointments) {
        if (toUpper(a.appointmentId) == query || toUpper(a.customerId) == query || toUpper(a.licensePlate) == query) {
            matches.push_back(&a);
        }
    }

    if (matches.empty()) {
        std::cout << Color::BRIGHT_YELLOW << "No matching appointments found.\n" << Color::RESET;
        return;
    }

    if (matches.size() == 1) {
        generateBookingSlip(*matches[0]);
        return;
    }

    std::cout << Color::BRIGHT_CYAN << "\nFound " << matches.size() << " matching appointment(s):\n" << Color::RESET;
    displayAppointmentHeader();
    for (size_t i = 0; i < matches.size(); ++i) {
        std::cout << Color::BRIGHT_CYAN << std::left << std::setw(5) << (std::to_string(i + 1) + ".");
        displayAppointmentRow(*matches[i]);
    }

    int sel = -1;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter the number (#) to view full details (0 to Return): " << Color::RESET;
    if (!(std::cin >> sel)) {
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        return;
    }
    if (sel == 0) return;
    if (sel < 1 || sel > static_cast<int>(matches.size())) {
        std::cout << Color::BRIGHT_RED << "Invalid selection.\n" << Color::RESET;
        return;
    }

    generateBookingSlip(*matches[sel - 1]);
}

void rescheduleAppointment(std::vector<Appointment>& appointments) {
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "\n--- Active Appointments ---\n" << Color::RESET;

    std::vector<Appointment*> activeApps;
    for (auto& a : appointments) {
        if (a.isActive) {
            activeApps.push_back(&a);
        }
    }

    if (activeApps.empty()) {
        std::cout << Color::BRIGHT_YELLOW << "No active appointments found.\n" << Color::RESET;
        return;
    }

    std::sort(activeApps.begin(), activeApps.end(), [](const Appointment* a, const Appointment* b) {
        return a->appointmentId < b->appointmentId;
        });

    displayAppointmentHeader();
    int index = 1;
    for (const auto* a : activeApps) {
        std::cout << Color::BRIGHT_CYAN << std::left << std::setw(5) << (std::to_string(index++) + ".");
        displayAppointmentRow(*a);
    }

    int choice = -1;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter choice (1-" << activeApps.size() << ") to reschedule (or 0 to Return to Main Menu): " << Color::RESET;

    if (!(std::cin >> choice)) {
        std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        return;
    }

    if (choice == 0) return;

    if (choice < 1 || choice > static_cast<int>(activeApps.size())) {
        std::cout << Color::BRIGHT_RED << "Invalid selection! Choice out of range.\n" << Color::RESET;
        return;
    }

    rescheduleSpecificAppointment(activeApps[choice - 1], appointments);
}

void cancelAppointment(std::vector<Appointment>& appointments, std::vector<Customer>& customers) {
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "\n--- Active Appointments ---\n" << Color::RESET;

    std::vector<Appointment*> activeApps;
    for (auto& a : appointments) {
        if (a.isActive) {
            activeApps.push_back(&a);
        }
    }

    if (activeApps.empty()) {
        std::cout << Color::BRIGHT_YELLOW << "No active appointments found.\n" << Color::RESET;
        return;
    }

    std::sort(activeApps.begin(), activeApps.end(), [](const Appointment* a, const Appointment* b) {
        return a->appointmentId < b->appointmentId;
        });

    displayAppointmentHeader();
    int index = 1;
    for (const auto* a : activeApps) {
        std::cout << Color::BRIGHT_CYAN << std::left << std::setw(5) << (std::to_string(index++) + ".");
        displayAppointmentRow(*a);
    }

    int choice = -1;
    std::cout << Color::BRIGHT_YELLOW << "\nEnter choice (1-" << activeApps.size() << ") to cancel (or 0 to Return to Main Menu): " << Color::RESET;

    if (!(std::cin >> choice)) {
        std::cout << Color::BRIGHT_RED << "Invalid input! Please enter a number.\n" << Color::RESET;
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        return;
    }

    if (choice == 0) return;

    if (choice < 1 || choice > static_cast<int>(activeApps.size())) {
        std::cout << Color::BRIGHT_RED << "Invalid selection! Choice out of range.\n" << Color::RESET;
        return;
    }

    Appointment* selectedApp = activeApps[choice - 1];

    char confirm;
    std::cout << Color::BRIGHT_YELLOW
        << "\nAre you sure you want to cancel Appointment ID " << selectedApp->appointmentId << "? (Y/N): "
        << Color::RESET;
    std::cin >> confirm;

    if (std::toupper(confirm) == 'Y') {
        cancelSpecificAppointment(selectedApp, customers);
        saveAppointmentsToFile(appointments);
    }
    else {
        std::cout << Color::BRIGHT_YELLOW << "Cancellation aborted.\n" << Color::RESET;
    }
}