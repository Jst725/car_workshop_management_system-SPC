#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "structures.h"
#include "file_storage.h"
#include "customer.h"
#include "appointment.h"
#include "service.h"
#include "billing.h"
#include "report.h"

// ANSI Color Code Definitions (Light theme without Blue, Green, or Purple)
namespace Color {
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";

    // Light / Bright Colors
    const std::string CYAN = "\033[36m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_WHITE = "\033[97m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string LIGHT_GRAY = "\033[37m";
}

// Displays a colorful terminal logo splash screen using light colors
static void showLogoWindow() {
    std::cout << "\n";
    std::cout << Color::CYAN << "=========================================================================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_YELLOW << Color::BOLD;
    std::cout << "    #####   ##   ##    ##   ##   #####  #####   ##  ##   ####   ##  ##   ####   #####    \n";
    std::cout << "   ##   ##  ##   ##    ##   ##  ##  ##  ##  ##  ## ##   ##      ##  ##  #    #  ##  ##    \n";
    std::cout << "   ##       ## # ##    ## # ##  ##  ##  #####   ####    ######  ######  #    #  #####     \n";
    std::cout << "   ##   ##  #######    #######  ##  ##  ##  ##  ## ##       ##  ##  ##  #    #  ##        \n";
    std::cout << "    #####    ## ##      ## ##    #####  ##  ##  ##  ##  #####   ##  ##   ####   ##        \n";
    std::cout << Color::RESET;
    std::cout << Color::CYAN << "=========================================================================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_CYAN << Color::BOLD;
    std::cout << "                       ______                                                           \n";
    std::cout << "                      /|_||_`.__                                                        \n";
    std::cout << "                     (   _    _ _\\                                                      \n";
    std::cout << "                     =`-(_)--(_)-'  " << Color::BRIGHT_YELLOW << "[ CAR WORKSHOP MANAGEMENT SYSTEM ]                  \n";
    std::cout << Color::RESET;
    std::cout << Color::CYAN << "=========================================================================================\n" << Color::RESET;
    std::cout << Color::BRIGHT_WHITE << Color::BOLD << "                                 Starting Application...                                \n" << Color::RESET;
    std::cout << Color::CYAN << "=========================================================================================\n\n" << Color::RESET;

    // Pause for 2.5 seconds to let the splash screen display
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
}

int main() {
    // Enable ANSI escape sequences for Windows Command Prompt / PowerShell
#ifdef _WIN32
    system("cls");
#endif

    // Show splash screen banner on startup
    showLogoWindow();

    std::vector<Customer> customers;
    std::vector<Appointment> appointments;
    std::vector<Invoice> invoices;

    // Load all saved data on startup
    loadAllData(customers, appointments, invoices);

    int choice = -1;
    do {
        std::cout << Color::CYAN << "\n==================================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << Color::BOLD << "  CAR WORKSHOP MANAGEMENT SYSTEM                    \n" << Color::RESET;
        std::cout << Color::CYAN << "==================================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_CYAN << "1. " << Color::BRIGHT_WHITE << "Customer Management\n";
        std::cout << Color::BRIGHT_CYAN << "2. " << Color::BRIGHT_WHITE << "Appointment Management\n";
        std::cout << Color::BRIGHT_CYAN << "3. " << Color::BRIGHT_WHITE << "Services Management\n";
        std::cout << Color::BRIGHT_CYAN << "4. " << Color::BRIGHT_WHITE << "Billing & Payment\n";
        std::cout << Color::BRIGHT_CYAN << "5. " << Color::BRIGHT_WHITE << "Reports & Analytics\n";
        std::cout << Color::BRIGHT_RED << "0. " << Color::BRIGHT_WHITE << "Exit System\n";
        std::cout << Color::CYAN << "==================================================\n" << Color::RESET;
        std::cout << Color::BRIGHT_YELLOW << "Enter your choice: " << Color::RESET;

        if (!(std::cin >> choice)) {
            std::cout << Color::BRIGHT_RED << "Invalid input! Input numbers only.\n" << Color::RESET;
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        switch (choice) {
        case 1:
            customerMenu(customers);
            break;
        case 2:
            appointmentMenu(appointments, customers);
            break;
        case 3:
            vehicleMenu(customers, appointments);
            break;
        case 4:
            billingMenu(invoices, customers, appointments);
            break;
        case 5:
            reportMenu(invoices, customers);
            break;
        case 0: {
            char confirm;
            std::cout << Color::BRIGHT_YELLOW << "Are you sure you want to exit? (Y/N): " << Color::RESET;
            std::cin >> confirm;
            if (confirm == 'Y' || confirm == 'y') {
                saveAllData(customers, appointments, invoices);
                std::cout << Color::BRIGHT_CYAN << Color::BOLD << "Exiting system. Goodbye!\n" << Color::RESET;
            }
            else {
                choice = -1;
            }
            break;
        }
        default:
            std::cout << Color::BRIGHT_RED << "Invalid choice! Select from 0 to 5.\n" << Color::RESET;
        }
    } while (choice != 0);

    return 0;
}