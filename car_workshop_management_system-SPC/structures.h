#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>

struct Vehicle {
    std::string licensePlate;
    std::string brand;
    std::string model;
    std::string lastServiceDate; // Format: YYYY-MM-DD
    std::string status;          // "Awaiting Parts", "In Progress", "Ready for Pickup"
    std::string assignedTechnician;
    std::vector<std::string> activeServiceLogs;
    double currentServiceCost = 0.0;
};

struct Customer {
    std::string id;
    std::string name;
    std::string phone;
    std::string membershipTier; // "Standard", "Silver", "Gold"
    std::vector<Vehicle> vehicles;
};

struct Appointment {
    std::string appointmentId;
    std::string customerId;
    std::string licensePlate;
    std::string date; // Format: YYYY-MM-DD
    std::string time; // Format: HH:MM
    std::string serviceType;
    bool isActive = true;
};

struct Invoice {
    std::string invoiceId;
    std::string customerId;
    std::string licensePlate;
    std::string date;
    double partsCost = 0.0;
    double laborCost = 0.0;
    double discountAmount = 0.0;
    double taxAmount = 0.0;
    double totalAmount = 0.0;
    std::string paymentType; // "Cash", "Card", "E-Wallet"
    bool isPaid = false;
};

#endif