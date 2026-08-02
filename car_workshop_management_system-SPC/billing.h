#pragma once

#include <string>
#include "global.h"

void createBill(std::string billID[], std::string billPlate[], double amount[], int& totalBills);
void displayBills(const std::string billID[], const std::string billPlate[], const double amount[], int totalBills);