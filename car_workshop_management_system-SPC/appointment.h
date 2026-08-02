#ifndef APPOINTMENT_H
#define APPOINTMENT_H

#include "structures.h"
#include <vector>

void appointmentMenu(std::vector<Appointment>& appointments, const std::vector<Customer>& customers);

void bookAppointment(std::vector<Appointment>& appointments, const std::vector<Customer>& customers);
void generateBookingSlip(const Appointment& app);
void displayAppointment(const std::vector<Appointment>& appointments);
void searchAppointment(const std::vector<Appointment>& appointments);
void rescheduleAppointment(std::vector<Appointment>& appointments);
void cancelAppointment(std::vector<Appointment>& appointments);

#endif