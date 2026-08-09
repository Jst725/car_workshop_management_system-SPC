#pragma once

#include "structures.h"
#include <vector>
#include <string>

// Customer file operations
void saveCustomersToFile(const std::vector<Customer>& customers);
void loadCustomersFromFile(std::vector<Customer>& customers);

// Appointment file operations
void saveAppointmentsToFile(const std::vector<Appointment>& appointments);
void loadAppointmentsFromFile(std::vector<Appointment>& appointments);

// Invoice file operations
void saveInvoicesToFile(const std::vector<Invoice>& invoices);
void loadInvoicesFromFile(std::vector<Invoice>& invoices);

// Load/Save all data
void saveAllData(const std::vector<Customer>& customers,
    const std::vector<Appointment>& appointments,
    const std::vector<Invoice>& invoices);

void loadAllData(std::vector<Customer>& customers,
    std::vector<Appointment>& appointments,
    std::vector<Invoice>& invoices);