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


struct InvoiceItem {
    std::string description; 
    std::string itemType;    
    int quantity = 1;
    double unitPrice = 0.0;
    double totalPrice = 0.0;
};

struct Vehicle {
    std::string licensePlate;
    std::string brand;
    std::string model;
    std::string status;            
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

    std::string name;
    std::string phone;
    std::string membershipTier = "Regular"; 
    double totalPurchases = 0.0;
    std::vector<Vehicle> vehicles;
};

struct Appointment {
    std::string appointmentId;
    std::string customerId; 
    std::string licensePlate;
    std::string date;         
    std::string time;         
    std::string serviceType;  
    double totalFee = 0.0;    
                               
    bool isActive = true;    
};

struct Invoice {
    std::string invoiceId;
    std::string customerId;
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

#endif 