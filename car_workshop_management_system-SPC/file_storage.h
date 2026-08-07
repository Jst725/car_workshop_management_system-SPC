#pragma once
#include "structures.h"
#include <vector>
#include <string>

void saveCustomersToFile(const std::vector<Customer>& customers);
void loadCustomersFromFile(std::vector<Customer>& customers);

void saveAppointmentsToFile(const std::vector<Appointment>& appointments);
void saveInvoicesToFile(const std::vector<Invoice>& invoices);
void saveAllData(const std::vector<Customer>& customers,
    const std::vector<Appointment>& appointments,
    const std::vector<Invoice>& invoices);