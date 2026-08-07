#ifndef BILLING_H
#define BILLING_H

#include "structures.h"
#include <vector>

void billingMenu(std::vector<Invoice>& invoices, std::vector<Customer>& customers);

void generateInvoice(std::vector<Invoice>& invoices, const std::vector<Customer>& customers);
void processPayment(std::vector<Invoice>& invoices, const std::vector<Customer>& customers);
void printReceipt(const Invoice& inv);
void paymentHistory(const std::vector<Invoice>& invoices);

#endif