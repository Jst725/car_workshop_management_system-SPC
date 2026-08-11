#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>

// Definition for individual parts used in service
struct PartItem {
    std::string partId;
    std::string name;
    double price = 0.0;
    int stockQuantity = 0;
};

// Definition for service logs or active task descriptions
struct ServiceLog {
    std::string date;
    std::string description;
    double cost = 0.0;
};

// Itemized detail structure to store specific service lines and parts on the invoice
struct InvoiceItem {
    std::string description; // e.g., Part name or labor task
    std::string itemType;    // "Part" or "Labor"
    int quantity = 1;
    double unitPrice = 0.0;
    double totalPrice = 0.0;
};

struct Vehicle {
    std::string licensePlate;
    std::string brand;
    std::string model;
    std::string status;              // e.g., "Ready for Pickup"
    std::string assignedTechnician;
    std::string lastServiceDate;

    std::string lastPartsUsed;
    std::string lastServiceDescription;

    double partsCost = 0.0;
    double laborCost = 0.0;
    double currentServiceCost = 0.0;
    std::vector<PartItem> parts;
    std::vector<ServiceLog> activeServiceLogs;
};

struct Customer {
    // NOTE: Customer ID removed - phone number is now the unique identifier
    // used everywhere (searching, appointments, invoices, deleting, etc.).
    std::string name;
    std::string phone;
    std::string membershipTier = "Regular"; // "Regular", "Silver", "Gold", "VIP"
    double totalPurchases = 0.0;
    std::vector<Vehicle> vehicles;
};

struct Appointment {
    std::string appointmentId;
    std::string customerId; // now holds the customer's PHONE NUMBER
    std::string licensePlate;
    std::string date;         // YYYY-MM-DD
    std::string time;         // HH:MM (24-hour)
    std::string serviceType;  // Description of service
    bool isActive = true;     // True = Confirmed, False = Cancelled
};

struct Invoice {
    std::string invoiceId;
    std::string customerId; // now holds the customer's PHONE NUMBER
    std::string licensePlate;
    std::string date;

    std::vector<InvoiceItem> items;

    double partsCost = 0.0;
    double laborCost = 0.0;
    double discountAmount = 0.0;
    double taxAmount = 0.0;
    double totalAmount = 0.0;
    bool isPaid = false;
    std::string paymentType;
};

#endif // STRUCTURES_H
