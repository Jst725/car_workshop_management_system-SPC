#ifndef BILLING_H
#define BILLING_H

#include "structures.h"
#include <vector>

void billingMenu(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments);
void generateInvoice(std::vector<Invoice>& invoices, std::vector<Customer>& customers, const std::vector<Appointment>& appointments);
void editInvoice(std::vector<Invoice>& invoices, std::vector<Customer>& customers);
void processPayment(std::vector<Invoice>& invoices, std::vector<Customer>& customers, std::vector<Appointment>& appointments);
void displayInvoices(const std::vector<Invoice>& invoices);
void viewCustomerHistory(const std::vector<Customer>& customers, const std::vector<Invoice>& invoices);
void paymentHistory(const std::vector<Customer>& customers, const std::vector<Invoice>& invoices);

#endif