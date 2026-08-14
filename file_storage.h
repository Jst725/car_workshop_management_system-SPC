#pragma once
#include "structures.h"
#include <vector>
#include <string>


void saveCustomersToFile(const std::vector<Customer>& customers);
void loadCustomersFromFile(std::vector<Customer>& customers);

// ---- Appointment module (best-effort implementation - Appointment module owner should verify) ----
void saveAppointmentsToFile(const std::vector<Appointment>& appointments);
void loadAppointmentsFromFile(std::vector<Appointment>& appointments);

// ---- Billing module (placeholder - Invoice.items makes this non-trivial;
//      Billing module owner still needs to implement this properly) ----
void saveInvoicesToFile(const std::vector<Invoice>& invoices);
void loadInvoicesFromFile(std::vector<Invoice>& invoices);

// ---- Combined helpers used by main.cpp ----
void loadAllData(std::vector<Customer>& customers,
    std::vector<Appointment>& appointments,
    std::vector<Invoice>& invoices);
void saveAllData(const std::vector<Customer>& customers,
    const std::vector<Appointment>& appointments,
    const std::vector<Invoice>& invoices);