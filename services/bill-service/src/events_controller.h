#ifndef EVENTS_CONTROLLER_H
#define EVENTS_CONTROLLER_H

#include <memory>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "database.h"
#include "auth_middleware.h"

using json = nlohmann::json;

class EventsController {
public:
    EventsController(std::shared_ptr<Database> db, std::shared_ptr<AuthMiddleware> auth);
    
    void getEvents(const httplib::Request& req, httplib::Response& res);
    void createEvent(const httplib::Request& req, httplib::Response& res);
    void getEvent(const httplib::Request& req, httplib::Response& res);
    void updateEvent(const httplib::Request& req, httplib::Response& res);
    void deleteEvent(const httplib::Request& req, httplib::Response& res);

private:
    std::shared_ptr<Database> db_;
    std::shared_ptr<AuthMiddleware> auth_;
    
    struct CreateEventRequest {
        std::string name;
        std::string description;
        std::string eventType;
        std::string startDate;
        std::string endDate;
    };
    
    struct UpdateEventRequest {
        std::string name;
        std::string description;
        std::string eventType;
        std::string status;
        std::string startDate;
        std::string endDate;
    };
    
    bool validateCreateEventRequest(const json& requestBody, CreateEventRequest& req, std::string& error);
    bool validateUpdateEventRequest(const json& requestBody, UpdateEventRequest& req, std::string& error);
    bool isValidEventType(const std::string& type);
    bool isValidEventStatus(const std::string& status);
    bool isValidDateFormat(const std::string& date);
    
    json createErrorResponse(const std::string& message, int statusCode = 400);
    json createSuccessResponse(const json& data = json::object());
};

#endif