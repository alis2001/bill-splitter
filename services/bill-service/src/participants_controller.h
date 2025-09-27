#ifndef PARTICIPANTS_CONTROLLER_H
#define PARTICIPANTS_CONTROLLER_H

#include <memory>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "database.h"
#include "auth_middleware.h"

using json = nlohmann::json;

class ParticipantsController {
public:
    ParticipantsController(std::shared_ptr<Database> db, std::shared_ptr<AuthMiddleware> auth);
    
    void getParticipants(const httplib::Request& req, httplib::Response& res);
    void addParticipant(const httplib::Request& req, httplib::Response& res);
    void updateParticipant(const httplib::Request& req, httplib::Response& res);
    void removeParticipant(const httplib::Request& req, httplib::Response& res);

private:
    std::shared_ptr<Database> db_;
    std::shared_ptr<AuthMiddleware> auth_;
    
    struct AddParticipantRequest {
        std::string userId;
        double sharePercentage;
        double customAmount;
    };
    
    struct UpdateParticipantRequest {
        double sharePercentage;
        double customAmount;
    };
    
    bool validateAddParticipantRequest(const json& requestBody, AddParticipantRequest& req, std::string& error);
    bool validateUpdateParticipantRequest(const json& requestBody, UpdateParticipantRequest& req, std::string& error);
    bool isValidPercentage(double percentage);
    bool isValidAmount(double amount);
    
    json createErrorResponse(const std::string& message, int statusCode = 400);
    json createSuccessResponse(const json& data = json::object());
};

#endif