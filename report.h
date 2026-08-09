#ifndef REPORT_H
#define REPORT_H

#include "structures.h"
#include <vector>

void reportMenu(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers);
void dailyReport(const std::vector<Invoice>& invoices);
void customerStatistics(const std::vector<Customer>& customers);
void topSpendingCustomers(const std::vector<Customer>& customers); // <--- ADDED DECLARATION
void exportReport(const std::vector<Invoice>& invoices);

#endif