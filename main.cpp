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

// Displays a pure C++ terminal logo splash screen (No external libraries required)
static void showLogoWindow() {
    std::cout << "\n";
    std::cout << "====================================================================================\n";
    std::cout << "                                                                                    \n";
    std::cout << "    #####   ##   ##    ##   ##   #####  #####   ##  ##   ####   ##  ##   #####    \n";
    std::cout << "   ##   ##  ##   ##    ##   ##  ##  ##  ##  ##  ## ##   ##  ##  ##  ##  ##  ##    \n";
    std::cout << "   ##       ## # ##    ## # ##  ##  ##  #####   ####    ######  ######  #####     \n";
    std::cout << "   ##   ##  #######    #######  ##  ##  ##  ##  ## ##   ##  ##  ##  ##  ##        \n";
    std::cout << "    #####    ## ##      ## ##    #####  ##  ##  ##  ##  ##  ##  ##  ##  ##        \n";
    std::cout << "                                                                                    \n";
    std::cout << "====================================================================================\n";
    std::cout << "                   ______                                                           \n";
    std::cout << "                  /|_||_`.__                                                        \n";
    std::cout << "                 (   _    _ _\\                                                      \n";
    std::cout << "                 =`-(_)--(_)-'  [ CAR WORKSHOP MANAGEMENT SYSTEM ]                  \n";
    std::cout << "====================================================================================\n";
    std::cout << "                             Starting Application...                                \n";
    std::cout << "====================================================================================\n\n";

    // Pause for 2.5 seconds to let the splash screen display
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));
}

int main() {
    // Show splash screen banner on startup
    showLogoWindow();

    std::vector<Customer> customers;
    std::vector<Appointment> appointments;
    std::vector<Invoice> invoices;

    // Load all saved data on startup
    loadAllData(customers, appointments, invoices);

    int choice = -1;
    do {
        std::cout << "\n==================================================\n";
        std::cout << "  CAR WORKSHOP MANAGEMENT SYSTEM                    \n";
        std::cout << "==================================================\n";
        std::cout << "1. Customer Management\n";
        std::cout << "2. Appointment Management\n";
        std::cout << "3. Services Management\n";
        std::cout << "4. Billing & Payment\n";
        std::cout << "5. Reports & Analytics\n";
        std::cout << "0. Exit System\n";
        std::cout << "==================================================\n";
        std::cout << "Enter your choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input! Input numbers only.\n";
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
            std::cout << "Are you sure you want to exit? (Y/N): ";
            std::cin >> confirm;
            if (confirm == 'Y' || confirm == 'y') {
                saveAllData(customers, appointments, invoices);
                std::cout << "Exiting system. Goodbye!\n";
            }
            else {
                choice = -1;
            }
            break;
        }
        default:
            std::cout << "Invalid choice! Select from 0 to 5.\n";
        }
    } while (choice != 0);

    return 0;
}