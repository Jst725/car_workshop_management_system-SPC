#ifndef VEHICLE_H
#define VEHICLE_H

#include "structures.h"
#include <vector>

void vehicleMenu(std::vector<Customer>& customers);

void updateVehicleStatus(std::vector<Customer>& customers);
void updateService(std::vector<Customer>& customers);
double calculateServicePrice(const std::vector<double>& parts, double labor);
void displayWorkshopVehicle(const std::vector<Customer>& customers);
void checkServiceReminders(const std::vector<Customer>& customers);

#endif