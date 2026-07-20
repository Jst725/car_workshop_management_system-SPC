#include <iostream>
#include <string>
#include "customer.h"
#include "appointment.h"
#include "vehicle.h"
#include "billing_report.h"

using namespace std;

void displayMenu(string name[], string plate[], string phone[], int& totalRecords, int SIZE) {
    int mainChoice;
    do {
        cout << "\n==================================================\n";
        cout << "        CAR WORKSHOP MANAGEMENT SYSTEM (MAIN)      \n";
        cout << "==================================================\n";
        cout << " 1. Customer management\n";
        cout << " 2. Appointment management\n";
        cout << " 3. Vehicle management\n";
        cout << " 4. Billing Management\n";
        cout << " 5. Report & Statistics\n";
        cout << " 0. Exit system\n";
        cout << "==================================================\n";
        cout << "Please enter your choice: ";
        cin >> mainChoice;

        switch (mainChoice) {
        case 1: customerManagementMenu(name, plate, phone, totalRecords, SIZE); break;
        case 2: appointmentManagement(name, plate, phone, totalRecords, SIZE); break;
        case 3: updateStatus(name, plate, phone, totalRecords); break;
        case 4: generateBill(name, plate, phone, totalRecords); break;
        case 5: viewReports(name, plate, phone, totalRecords); break;
        case 0: cout << "Exiting system. Goodbye!\n"; break;
        default: cout << "[ERROR] Invalid selection!\n";
        }
    } while (mainChoice != 0);
}

int main() {
    const int SIZE = 100;
    string name[SIZE];
    string plate[SIZE];
    string phone[SIZE];
    int totalRecords = 0;

    displayMenu(name, plate, phone, totalRecords, SIZE);
    return 0;
}