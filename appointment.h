#ifndef APPOINTMENT_H
#define APPOINTMENT_H

#include <vector>
#include "structures.h"

void appointmentMenu(std::vector<Appointment>& appointments, std::vector<Customer>& customers);
void bookAppointment(std::vector<Appointment>& appointments, std::vector<Customer>& customers);
void generateBookingSlip(const Appointment& app);
void viewAppointmentsMenu(std::vector<Appointment>& appointments);
void viewAllAppointments(std::vector<Appointment>& appointments);
void viewAppointmentsByDate(std::vector<Appointment>& appointments);
void viewTodaysAppointments(std::vector<Appointment>& appointments);
void searchAppointment(const std::vector<Appointment>& appointments);
void rescheduleAppointment(std::vector<Appointment>& appointments);
void cancelAppointment(std::vector<Appointment>& appointments);
void filterAppointmentsByServiceType(const std::vector<Appointment>& appointments);

#endif