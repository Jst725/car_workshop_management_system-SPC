#pragma once
#include "structures.h"
#include <vector>
#include <string>

// ---- Customer module (owned by me) ----
// saveCustomersToFile() writes BOTH files:
//   customers.txt -> phone|name|membershipTier|totalPurchases
//   vehicles.txt  -> phone|plate|brand|model|lastServiceDate|status|assignedTechnician|
//                     partsCost|laborCost|lastServiceDescription|lastPartsUsed|currentServiceCost
// loadCustomersFromFile() reads both files back and re-links each vehicle to
// its owner via the phone number. It also auto-migrates older file formats
// from earlier versions of this project (with Customer ID, or missing fields)
// so existing data is never lost.
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
