#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "structures.h"
#include <vector>
#include <string>

void customerMenu(std::vector<Customer>& customers,
    std::vector<Appointment>& appointments,
    std::vector<Invoice>& invoices);

// Core customer operations
void registerCustomer(std::vector<Customer>& customers);
void addVehicleToCustomer(std::vector<Customer>& customers);
void viewCustomers(const std::vector<Customer>& customers);
void searchCustomer(const std::vector<Customer>& customers);
void deleteCustomer(std::vector<Customer>& customers,
    const std::vector<Appointment>& appointments,
    const std::vector<Invoice>& invoices);

// Validation helpers
bool isValidName(const std::string& str);
bool isValidPhone(const std::string& str);
bool isValidPlate(const std::string& str);
bool isPlateTaken(const std::vector<Customer>& customers, const std::string& plate);
bool isValidVehicleText(const std::string& str);

#endif