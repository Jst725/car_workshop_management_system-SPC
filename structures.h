#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>

// Definition for individual parts used in service
struct PartItem {
    std::string partId;
    std::string name;       
    double price;
    int stockQuantity;
};

struct ServiceLog {
    std::string date;
    std::string description;
    double cost;
};

struct Vehicle {
    std::string licensePlate;
    std::string brand;
    std::string model;
    std::string status;              // e.g., "Ready for Pickup"
    std::string assignedTechnician;
    std::string lastServiceDate;
    double partsCost;
    double laborCost;
    double currentServiceCost;
    std::vector<PartItem> parts;             // Updated to use PartItem
    std::vector<ServiceLog> activeServiceLogs; // Added active service logs
};

struct Customer {
    std::string id;
    std::string name;
    std::string phone;
    std::string membershipTier;     // "Standard", "Silver", "Gold", "VIP"
    double totalPurchases;
    std::vector<Vehicle> vehicles;
};

struct Appointment {
    std::string appointmentId;
    std::string customerId;
    std::string licensePlate;
    std::string date;         // YYYY-MM-DD
    std::string time;         // HH:MM (24-hour)
    std::string serviceType;  // Description of service
    bool isActive;            // True = Confirmed, False = Cancelled
};

struct Invoice {
    std::string invoiceId;
    std::string customerId;
    std::string licensePlate;
    std::string date;
    double partsCost;
    double laborCost;
    double discountAmount;
    double taxAmount;
    double totalAmount;
    bool isPaid;
    std::string paymentType;
};

#endif // STRUCTURES_H