#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "structures.h"
#include <vector>
#include <string>

// Main menu for the Customer Management module (matches main.cpp: customerMenu(customers))
void customerMenu(std::vector<Customer>& customers);

// Core customer operations (all keyed by PHONE NUMBER, no customer ID anymore)
void registerCustomer(std::vector<Customer>& customers);
void addVehicleToCustomer(std::vector<Customer>& customers);
void viewCustomers(const std::vector<Customer>& customers);
void searchCustomer(const std::vector<Customer>& customers);
void deleteCustomer(std::vector<Customer>& customers);

// Validation helpers
bool isValidName(const std::string& str);
bool isValidPhone(const std::string& str);
bool isValidPlate(const std::string& str);
bool isValidLicensePlate(const std::string& str); // alias used by appointment.cpp
bool isValidVehicleText(const std::string& str);
bool isPlateTaken(const std::vector<Customer>& customers, const std::string& plate);

// String helper
std::string toUpper(std::string str);

#endif