#ifndef REPORT_H
#define REPORT_H

#include <vector>
#include "structures.h"

// Report Menu Functions
void reportMenu(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers);
void allReportsSummaryMenu(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers);
void dailyReport(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers);
void monthlyReport(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers);
void yearlyReport(const std::vector<Invoice>& invoices);
void salesAnalysisMenu(const std::vector<Invoice>& invoices);
void exportReports(const std::vector<Invoice>& invoices, const std::vector<Customer>& customers);

#endif // REPORT_H