#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "structures.h"
#include <vector>
#include <string>

// Core Customer Functions
void customerMenu(std::vector<Customer>& customers);
void addCustomer(std::vector<Customer>& customers);
void viewCustomers(const std::vector<Customer>& customers);
void searchCustomer(const std::vector<Customer>& customers);
void updateCustomer(std::vector<Customer>& customers);
void deleteCustomer(std::vector<Customer>& customers);

// Loyalty Tier Functions
void updateCustomerTier(Customer& customer);
double getTierDiscountRate(const std::string& tier);

// Input & Verification Helper Checks
bool isValidName(const std::string& name);
bool isValidPhone(const std::string& phone);
bool isPhoneExists(const std::vector<Customer>& customers, const std::string& phone, const std::string& currentCustId = "");
bool isValidLicensePlate(const std::string& plate);
bool isLicensePlateExists(const std::vector<Customer>& customers, const std::string& plate);

#endif