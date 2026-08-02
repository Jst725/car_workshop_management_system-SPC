#include "vehicle.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <string>

void vehicleMenu(std::vector<Customer>& customers) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "         VEHICLE MANAGEMENT             \n";
        std::cout << "========================================\n";
        std::cout << "1. Update Vehicle Status\n";
        std::cout << "2. Update Service & Pricing\n";
        std::cout << "3. View Workshop Vehicles Dashboard\n";
        std::cout << "4. Check Service Reminders\n";
        std::cout << "0. Return to Main Menu\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Enter choice: ";

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input!\n";
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        switch (choice) {
        case 1: updateVehicleStatus(customers); saveCustomersToFile(customers); break;
        case 2: updateService(customers); saveCustomersToFile(customers); break;
        case 3: displayWorkshopVehicle(customers); break;
        case 4: checkServiceReminders(customers); break;
        case 0: std::cout << "Returning to Main Menu...\n"; break;
        default: std::cout << "Invalid choice!\n";
        }
    } while (choice != 0);
}

void updateVehicleStatus(std::vector<Customer>& customers) {
    std::string plate;
    std::cout << "\nEnter Vehicle License Plate (or 0 to Return to Main Menu): ";
    std::cin >> plate;
    if (plate == "0") return;

    for (auto& c : customers) {
        for (auto& v : c.vehicles) {
            if (v.licensePlate == plate) {
                std::cout << "\nSelect Current Status:\n";
                std::cout << "1. Awaiting Parts\n";
                std::cout << "2. In Progress\n";
                std::cout << "3. Ready for Pickup\n";
                std::cout << "0. Return to Main Menu\n";
                std::cout << "Choice: ";
                int st;
                std::cin >> st;
                if (st == 0) return;
                switch (st) {
                case 1: v.status = "Awaiting Parts"; break;
                case 2: v.status = "In Progress"; break;
                case 3: v.status = "Ready for Pickup"; break;
                default: std::cout << "Invalid status choice.\n"; return;
                }

                std::cout << "Assign Technician Name: ";
                std::cin.ignore(1000, '\n');
                std::getline(std::cin, v.assignedTechnician);

                std::cout << "Status updated for " << v.licensePlate << "!\n";
                return;
            }
        }
    }
    std::cout << "Vehicle plate not found.\n";
}

double calculateServicePrice(const std::vector<double>& parts, double labor) {
    double total = labor;
    for (double p : parts) total += p;
    return total;
}

void updateService(std::vector<Customer>& customers) {
    std::string plate;
    std::cout << "\nEnter Vehicle License Plate (or 0 to Return to Main Menu): ";
    std::cin >> plate;
    if (plate == "0") return;

    for (auto& c : customers) {
        for (auto& v : c.vehicles) {
            if (v.licensePlate == plate) {
                int partCount = 0;
                std::cout << "Enter total number of replaced parts (0 to Return to Main Menu): ";
                std::cin >> partCount;
                if (partCount == 0) return;

                std::vector<double> partPrices;
                for (int i = 0; i < partCount; ++i) {
                    std::string task;
                    double price;
                    std::cout << "Enter Part Name #" << (i + 1) << ": ";
                    std::cin.ignore(1000, '\n');
                    std::getline(std::cin, task);
                    std::cout << "Enter Price for " << task << ": $";
                    std::cin >> price;

                    v.activeServiceLogs.push_back(task);
                    partPrices.push_back(price);
                }

                double laborCost = 0.0;
                std::cout << "Enter Flat Labor Charge: $";
                std::cin >> laborCost;

                v.currentServiceCost = calculateServicePrice(partPrices, laborCost);
                std::cout << "Calculated Service Cost: $" << std::fixed << std::setprecision(2) << v.currentServiceCost << "\n";
                return;
            }
        }
    }
    std::cout << "Vehicle not found.\n";
}

void displayWorkshopVehicle(const std::vector<Customer>& customers) {
    std::cout << "\n-----------------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(12) << "Plate"
        << std::setw(16) << "Vehicle"
        << std::setw(18) << "Status"
        << std::setw(18) << "Technician"
        << "Accrued Cost ($)\n";
    std::cout << "-----------------------------------------------------------------------------------------------\n";

    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            std::cout << std::left << std::setw(12) << v.licensePlate
                << std::setw(16) << (v.brand + " " + v.model)
                << std::setw(18) << v.status
                << std::setw(18) << v.assignedTechnician
                << std::fixed << std::setprecision(2) << v.currentServiceCost << "\n";
        }
    }
}

void checkServiceReminders(const std::vector<Customer>& customers) {
    std::cout << "\n=======================================================\n";
    std::cout << "     AUTOMATED SERVICE REMINDER DASHBOARD               \n";
    std::cout << "=======================================================\n";

    bool overdueFound = false;
    for (const auto& c : customers) {
        for (const auto& v : c.vehicles) {
            if (v.lastServiceDate < "2026-02-01") {
                std::cout << "[OVERDUE FLAG] " << c.name << " (" << c.phone << ")"
                    << " | Car: " << v.licensePlate << " (" << v.brand << ")"
                    << " | Last Serviced: " << v.lastServiceDate << "\n";
                overdueFound = true;
            }
        }
    }
    if (!overdueFound) std::cout << "All registered vehicles are up to date on service schedules!\n";
}