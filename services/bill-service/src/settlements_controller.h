#ifndef SETTLEMENTS_CONTROLLER_H
#define SETTLEMENTS_CONTROLLER_H

#include <memory>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "database.h"
#include "auth_middleware.h"

using json = nlohmann::json;

class SettlementsController {
public:
    SettlementsController(std::shared_ptr<Database> db, std::shared_ptr<AuthMiddleware> auth);
    
    // Get settlement summary for an event
    void getEventSettlements(const httplib::Request& req, httplib::Response& res);
    
    // Record a payment between users
    void recordPayment(const httplib::Request& req, httplib::Response& res);
    
    // Get user's overall balance across all events
    void getUserBalance(const httplib::Request& req, httplib::Response& res);

private:
    std::shared_ptr<Database> db_;
    std::shared_ptr<AuthMiddleware> auth_;
    
    json createErrorResponse(const std::string& message, int statusCode = 400);
    json createSuccessResponse(const json& data = json::object());
};

#endif