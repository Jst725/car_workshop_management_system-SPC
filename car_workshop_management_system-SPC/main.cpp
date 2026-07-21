#include <iostream>
#include <string>
#include "global.h"
#include "customer.h"
#include "vehicle.h"
#include "appointment.h"
#include "billing.h"
#include "report.h"

using namespace std;

int main() {
    static string customerNames[SIZE];
    static string customerPlates[SIZE];
    static string customerPhones[SIZE];
    static int totalCustomerRecords = 0;

    static string billIDs[SIZE];
    static string billPlates[SIZE];
    static double billAmounts[SIZE];
    static int totalBillRecords = 0;

    string inputChoice;
    int choice = 0;

    do {
        cout << "\n===================================" << endl;
        cout << "      Car Workshop System          " << endl;
        cout << "===================================" << endl;
        cout << "1. Customer Management" << endl;
        cout << "2. Vehicle Management" << endl;
        cout << "3. Appointment Management" << endl;
        cout << "4. Billing System" << endl;
        cout << "5. Generate Report" << endl;
        cout << "6. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> inputChoice;

        if (inputChoice.length() == 1 && isdigit(inputChoice[0])) {
            choice = inputChoice[0] - '0';
        }
        else {
            choice = -1;
        }

        switch (choice) {
        case 1:
            customerManagementMenu(customerNames, customerPlates, customerPhones, totalCustomerRecords, SIZE);
            break;
        case 2:
            updateStatus(customerNames, customerPlates, customerPhones, totalCustomerRecords);
            break;
        case 3:
            appointmentManagement(customerNames, customerPlates, customerPhones, totalCustomerRecords, SIZE);
            break;
        case 4:
            createBill(billIDs, billPlates, billAmounts, totalBillRecords);
            break;
        case 5:
            generateReport(billIDs, billPlates, billAmounts, totalBillRecords);
            break;
        case 6:
            cout << "Exiting system. Goodbye!" << endl;
            break;
        default:
            cout << "[ERROR] Invalid choice! Please enter a number (1-6).\n" << endl;
        }
    } while (choice != 6);

    return 0;
}