#include <iostream>
#include <string>
#include <cctype>
#include <limits>
#include "customer.h"

using namespace std;

bool isValidName(string str) {
    if (str.empty()) return false;
    for (int i = 0; i < str.length(); i++) {
        if (!isalpha(str[i]) && str[i] != ' ') {
            return false;
        }
    }
    return true;
}

bool isValidPlate(string str) {
    bool hasLetter = false;
    bool hasDigit = false;
    for (int i = 0; i < str.length(); i++) {
        if (isalpha(str[i])) hasLetter = true;
        else if (isdigit(str[i])) hasDigit = true;
        else return false;
    }
    return (hasLetter && hasDigit);
}

void addRecord(string name[], string plate[], string phone[], int& totalRecords, int SIZE) {
    cout << "\n>>> REGISTER NEW CUSTOMER <<<\n";
    if (totalRecords >= SIZE) {
        cout << "[ERROR] System database is full!\n";
        return;
    }

    string inputPhone;
    bool isDuplicate = false;

    while (true) {
        cout << "Enter Phone Number (Unique ID): ";
        cin >> inputPhone;

        bool allDigits = true;
        for (int i = 0; i < inputPhone.length(); i++) {
            if (!isdigit(inputPhone[i])) {
                allDigits = false;
                break;
            }
        }

        if (!allDigits) {
            cout << "[ERROR] Invalid Phone Number! Digits only (0-9).\n\n";
            continue;
        }

        if (inputPhone.length() < 10 || inputPhone.length() > 11) {
            cout << "[ERROR] Invalid length! Phone number must be 10 or 11 digits.\n\n";
            continue;
        }

        isDuplicate = false;
        for (int i = 0; i < totalRecords; i++) {
            if (phone[i] == inputPhone) {
                isDuplicate = true;
                break;
            }
        }
        if (isDuplicate) {
            cout << "[ERROR] This phone number is ALREADY REGISTERED!\n\n";
            return;
        }
        else {
            break;
        }
    }

    string inputName;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    while (true) {
        cout << "Enter Customer Name (Letters Only): ";
        getline(cin, inputName);
        if (isValidName(inputName)) {
            name[totalRecords] = inputName;
            break;
        }
        else {
            cout << "[ERROR] Invalid Name! Name must contain letters only.\n\n";
        }
    }

    string inputPlate;
    while (true) {
        cout << "Enter Vehicle Plate Number (Must mix Letters & Numbers): ";
        cin >> inputPlate;
        if (isValidPlate(inputPlate)) {
            plate[totalRecords] = inputPlate;
            break;
        }
        else {
            cout << "[ERROR] Invalid Plate! Plate cannot be numbers only, letters only, or contain special symbols.\n\n";
        }
    }

    phone[totalRecords] = inputPhone;
    totalRecords++;
    cout << "--------------------------------------------------\n";
    cout << "[SUCCESS] Customer & Vehicle Registered Successfully!\n";
}

void viewRecords(string name[], string plate[], string phone[], int& totalRecords) {
    cout << "\n>>> CUSTOMER NAMELIST <<<\n";
    if (totalRecords == 0) {
        cout << "No customer records found.\n";
        return;
    }

    cout << "No.\tCustomer Name\t\tPhone (ID)\t\tPlate Number\n";
    cout << "---------------------------------------------------------------------\n";
    for (int i = 0; i < totalRecords; i++) {
        cout << i + 1 << "\t" << name[i] << "\t\t" << phone[i] << "\t\t" << plate[i] << "\n";
    }
    cout << "---------------------------------------------------------------------\n";

    string inputSubChoice;
    int subChoice = 0;
    cout << "\n[ View Namelist Menu ]\n";
    cout << "1. Delete Customer Record\n";
    cout << "2. Back to Customer Menu\n";
    cout << "Enter your choice: ";
    cin >> inputSubChoice;

    if (inputSubChoice == "1") {
        int indexToDelete;
        cout << "Enter the Number (No.) to delete: ";
        cin >> indexToDelete;

        if (cin.fail() || indexToDelete < 1 || indexToDelete > totalRecords) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "[ERROR] Invalid record number.\n";
            return;
        }

        int targetIndex = indexToDelete - 1;
        for (int i = targetIndex; i < totalRecords - 1; i++) {
            name[i] = name[i + 1];
            phone[i] = phone[i + 1];
            plate[i] = plate[i + 1];
        }
        totalRecords--;
        cout << "[SUCCESS] Record deleted successfully!\n";
    }
}

void searchRecord(string name[], string plate[], string phone[], int totalRecords) {
    cout << "\n>>> ADD VEHICLE TO EXISTING CUSTOMER <<<\n";
    if (totalRecords == 0) {
        cout << "No registered customers in the system yet.\n";
        return;
    }

    string searchPhone;
    cout << "Search bar (Enter customer phone number): ";
    cin >> searchPhone;

    int foundIndex = -1;
    for (int i = 0; i < totalRecords; i++) {
        if (phone[i] == searchPhone) {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex == -1) {
        cout << "[ERROR] Customer phone number not found! Please register first.\n";
    }
    else {
        cout << "----------------------------------\n";
        cout << "Customer Found: " << name[foundIndex] << "\n";
        cout << "Current Plate: " << plate[foundIndex] << "\n";
        cout << "----------------------------------\n";

        char confirm;
        cout << "Is this the correct customer? (Y/N): ";
        cin >> confirm;

        if (confirm == 'Y' || confirm == 'y') {
            string inputPlate;
            while (true) {
                cout << "Enter NEW Vehicle Plate Number: ";
                cin >> inputPlate;
                if (isValidPlate(inputPlate)) {
                    plate[foundIndex] = inputPlate;
                    cout << "[SUCCESS] Vehicle plate updated/linked to this customer.\n";
                    break;
                }
                else {
                    cout << "[ERROR] Invalid Plate! Must mix letters and numbers.\n";
                }
            }
        }
        else {
            cout << "Operation cancelled.\n";
        }
    }
}

void customerManagementMenu(string name[], string plate[], string phone[], int& totalRecords, int SIZE) {
    string inputChoice;
    int choice = 0;
    do {
        cout << "\n==================================================\n";
        cout << "         CUSTOMER MANAGEMENT SYSTEM\n";
        cout << "==================================================\n";
        cout << " 1. View namelist (Includes Delete)\n";
        cout << " 2. Register customer\n";
        cout << " 3. Add vehicle\n";
        cout << " 4. Back to Main Menu\n";
        cout << "==================================================\n";
        cout << "Please enter your choice (1-4): ";
        cin >> inputChoice;

        if (inputChoice.length() == 1 && isdigit(inputChoice[0])) {
            choice = inputChoice[0] - '0';
        }
        else {
            choice = -1;
        }

        switch (choice) {
        case 1: viewRecords(name, plate, phone, totalRecords); break;
        case 2: addRecord(name, plate, phone, totalRecords, SIZE); break;
        case 3: searchRecord(name, plate, phone, totalRecords); break;
        case 4: cout << "Returning to Main Menu...\n"; break;
        default: cout << "[ERROR] Invalid choice! Please enter 1-4.\n";
        }
    } while (choice != 4);
}