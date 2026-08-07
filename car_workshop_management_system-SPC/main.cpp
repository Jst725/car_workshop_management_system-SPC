#include <iostream>
#include <vector>
#include "structures.h"
#include "file_storage.h"
#include "customer.h"
#include "appointment.h"
#include "vehicle.h"
#include "billing.h"
#include "report.h"

int main() {
    std::vector<Customer> customers;
    std::vector<Appointment> appointments;
    std::vector<Invoice> invoices;

    static string billIDs[SIZE];
    static string billPlates[SIZE];
    static double billAmounts[SIZE];
    static int totalBillRecords = 0;

    // ADDED: Automatically load customer data from txt file on program startup
    loadCustomers(customerNames, customerPlates, customerPhones, totalCustomerRecords);

    string inputChoice;
    int choice = 0;

    int choice = -1;
    do {
        std::cout << "\n==================================================\n";
        std::cout << "  CAR WORKSHOP MANAGEMENT SYSTEM (TEXT FILE SAVING)\n";
        std::cout << "==================================================\n";
        std::cout << "1. Customer Management\n";
        std::cout << "2. Appointment Management\n";
        std::cout << "3. Vehicle Management\n";
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
            customerMenu(customers, appointments, invoices);
            break;
        case 2:
            appointmentMenu(appointments, customers);
            break;
        case 3:
            vehicleMenu(customers);
            break;
        case 4:
            billingMenu(invoices, customers);
            break;
        case 5:
            reportMenu(invoices, customers);
            break;
        case 0: {
            char confirm;
            std::cout << "Are you sure you want to exit? (Y/N): ";
            std::cin >> confirm;
            if (confirm == 'Y' || confirm == 'y') {
                // Save data back to files before shutting down
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