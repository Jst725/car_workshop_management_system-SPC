#ifndef VEHICLE_H
#define VEHICLE_H

#include "structures.h"
#include <vector>
#include <string>

void vehicleMenu(std::vector<Customer>& customers, const std::vector<Appointment>& appointments);
void updateVehicleStatus(std::vector<Customer>& customers, const std::vector<Appointment>& appointments);
void updateService(std::vector<Customer>& customers, const std::vector<Appointment>& appointments);
void viewServiceBreakdown(const std::vector<Customer>& customers, const std::vector<Appointment>& appointments);
double calculateServicePrice(const std::vector<double>& parts, double labor);
void displayWorkshopVehicle(const std::vector<Customer>& customers, const std::vector<Appointment>& appointments);
void checkServiceReminders(const std::vector<Customer>& customers, const std::vector<Appointment>& appointments);
void resetVehicleServiceData(Vehicle& vehicle);

#endif