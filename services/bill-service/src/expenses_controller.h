#ifndef EXPENSES_CONTROLLER_H
#define EXPENSES_CONTROLLER_H

#include <memory>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "database.h"
#include "auth_middleware.h"

using json = nlohmann::json;

class ExpensesController {
public:
    ExpensesController(std::shared_ptr<Database> db, std::shared_ptr<AuthMiddleware> auth);
    
    void getExpenses(const httplib::Request& req, httplib::Response& res);
    void createExpense(const httplib::Request& req, httplib::Response& res);
    void getExpense(const httplib::Request& req, httplib::Response& res);
    void updateExpense(const httplib::Request& req, httplib::Response& res);
    void deleteExpense(const httplib::Request& req, httplib::Response& res);

private:
    std::shared_ptr<Database> db_;
    std::shared_ptr<AuthMiddleware> auth_;
    
    struct CreateExpenseRequest {
        std::string payerId;
        double amount;
        std::string description;
        std::string splitType;
        std::string expenseDate;
    };
    
    struct UpdateExpenseRequest {
        double amount;
        std::string description;
        std::string splitType;
        std::string expenseDate;
    };
    
    bool validateCreateExpenseRequest(const json& requestBody, CreateExpenseRequest& req, std::string& error);
    bool validateUpdateExpenseRequest(const json& requestBody, UpdateExpenseRequest& req, std::string& error);
    bool isValidSplitType(const std::string& type);
    bool isValidAmount(double amount);
    bool isValidDateFormat(const std::string& date);
    
    json createErrorResponse(const std::string& message, int statusCode = 400);
    json createSuccessResponse(const json& data = json::object());
};

#endif