#include "appointment.h"
#include "file_storage.h"
#include <iostream>
#include <iomanip>
#include <string>

void appointmentMenu(std::vector<Appointment>& appointments, const std::vector<Customer>& customers) {
    int choice = -1;
    do {
        std::cout << "\n========================================\n";
        std::cout << "       APPOINTMENT MANAGEMENT           \n";
        std::cout << "========================================\n";
        std::cout << "1. Book Appointment\n";
        std::cout << "2. View All Appointments\n";
        std::cout << "3. Search Appointment\n";
        std::cout << "4. Reschedule Appointment\n";
        std::cout << "5. Cancel Appointment\n";
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
        case 1: bookAppointment(appointments, customers); saveAppointmentsToFile(appointments); break;
        case 2: displayAppointment(appointments); break;
        case 3: searchAppointment(appointments); break;
        case 4: rescheduleAppointment(appointments); saveAppointmentsToFile(appointments); break;
        case 5: cancelAppointment(appointments); saveAppointmentsToFile(appointments); break;
        case 0: std::cout << "Returning to Main Menu...\n"; break;
        default: std::cout << "Invalid choice!\n";
        }
    } while (choice != 0);
}

void bookAppointment(std::vector<Appointment>& appointments, const std::vector<Customer>& customers) {
    Appointment app;
    std::cout << "\nEnter Customer ID (or 0 to Return to Main Menu): ";
    std::cin >> app.customerId;
    if (app.customerId == "0") return;

    bool custExists = false;
    for (const auto& c : customers) {
        if (c.id == app.customerId) {
            custExists = true;
            break;
        }
    }
    if (!custExists) {
        std::cout << "ERROR: Customer ID not found!\n";
        return;
    }

    std::cout << "Enter Vehicle License Plate: ";
    std::cin >> app.licensePlate;
    std::cout << "Enter Service Date (YYYY-MM-DD): ";
    std::cin >> app.date;
    std::cout << "Enter Time Slot (HH:MM): ";
    std::cin >> app.time;

    for (const auto& a : appointments) {
        if (a.isActive && a.date == app.date && a.time == app.time) {
            std::cout << "ERROR: Slot already booked!\n";
            return;
        }
    }

    std::cout << "Enter Service Description: ";
    std::cin.ignore(1000, '\n');
    std::getline(std::cin, app.serviceType);

    app.appointmentId = "APP" + std::to_string(appointments.size() + 101);
    app.isActive = true;
    appointments.push_back(app);

    std::cout << "\nBooking successful!\n";
    generateBookingSlip(app);
}

void generateBookingSlip(const Appointment& app) {
    std::cout << "\n----------------------------------------\n";
    std::cout << "          APPOINTMENT CONFIRMATION      \n";
    std::cout << "----------------------------------------\n";
    std::cout << "Booking ID    : " << app.appointmentId << "\n";
    std::cout << "Customer ID   : " << app.customerId << "\n";
    std::cout << "Vehicle Plate : " << app.licensePlate << "\n";
    std::cout << "Schedule      : " << app.date << " @ " << app.time << "\n";
    std::cout << "Service Type  : " << app.serviceType << "\n";
    std::cout << "Status        : " << (app.isActive ? "CONFIRMED" : "CANCELLED") << "\n";
    std::cout << "----------------------------------------\n";
}

void displayAppointment(const std::vector<Appointment>& appointments) {
    if (appointments.empty()) {
        std::cout << "\nNo appointments found.\n";
        return;
    }

    std::cout << "\n----------------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(10) << "Appt ID"
        << std::setw(12) << "Cust ID"
        << std::setw(12) << "Plate"
        << std::setw(14) << "Date"
        << std::setw(10) << "Time"
        << "Status\n";
    std::cout << "----------------------------------------------------------------------------------------\n";
    for (const auto& a : appointments) {
        std::cout << std::left << std::setw(10) << a.appointmentId
            << std::setw(12) << a.customerId
            << std::setw(12) << a.licensePlate
            << std::setw(14) << a.date
            << std::setw(10) << a.time
            << (a.isActive ? "Active" : "Cancelled") << "\n";
    }
}

void searchAppointment(const std::vector<Appointment>& appointments) {
    std::string query;
    std::cout << "\nEnter Booking ID or Customer ID (or 0 to Return to Main Menu): ";
    std::cin >> query;
    if (query == "0") return;

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
    std::string appId;
    std::cout << "\nEnter Appointment ID to reschedule (or 0 to Return to Main Menu): ";
    std::cin >> appId;
    if (appId == "0") return;

    for (auto& a : appointments) {
        if (a.appointmentId == appId && a.isActive) {
            std::string newDate, newTime;
            std::cout << "Enter New Date (YYYY-MM-DD): ";
            std::cin >> newDate;
            std::cout << "Enter New Time (HH:MM): ";
            std::cin >> newTime;

            for (const auto& chk : appointments) {
                if (chk.isActive && chk.date == newDate && chk.time == newTime) {
                    std::cout << "ERROR: Destination slot is already occupied!\n";
                    return;
                }
            }
            a.date = newDate;
            a.time = newTime;
            std::cout << "Appointment updated successfully!\n";
            return;
        }
    }
    std::cout << "Active appointment ID not found.\n";
}

void cancelAppointment(std::vector<Appointment>& appointments) {
    std::string appId;
    std::cout << "\nEnter Appointment ID to cancel (or 0 to Return to Main Menu): ";
    std::cin >> appId;
    if (appId == "0") return;

    for (auto& a : appointments) {
        if (a.appointmentId == appId) {
            a.isActive = false;
            std::cout << "Appointment successfully cancelled.\n";
            return;
        }
    }
    std::cout << "Appointment ID not found.\n";
}