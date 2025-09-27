#include "events_controller.h"
#include "utils.h"
#include <iostream>
#include <regex>
#include <set>

EventsController::EventsController(std::shared_ptr<Database> db, std::shared_ptr<AuthMiddleware> auth)
    : db_(db), auth_(auth) {}

void EventsController::getEvents(const httplib::Request& req, httplib::Response& res) {
    try {
        // Authenticate user
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Get events for user
        json events = db_->getEventsByUser(authResult.userId);
        
        json response = createSuccessResponse();
        response["events"] = events;
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to retrieve events: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void EventsController::createEvent(const httplib::Request& req, httplib::Response& res) {
    try {
        // Authenticate user
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Parse request body
        json requestBody;
        try {
            requestBody = json::parse(req.body);
        } catch (const std::exception& e) {
            json errorResponse = createErrorResponse("Invalid JSON format");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Validate request
        CreateEventRequest eventReq;
        std::string validationError;
        if (!validateCreateEventRequest(requestBody, eventReq, validationError)) {
            json errorResponse = createErrorResponse(validationError);
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Create event
        json event = db_->createEvent(
            authResult.userId,
            eventReq.name,
            eventReq.description,
            eventReq.eventType,
            eventReq.startDate,
            eventReq.endDate
        );

        json response = createSuccessResponse();
        response["event"] = event;
        
        res.status = 201;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to create event: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void EventsController::getEvent(const httplib::Request& req, httplib::Response& res) {
    try {
        // Authenticate user
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Extract event ID from URL
        std::string eventId = req.matches[1];
        
        if (!isValidUUID(eventId)) {
            json errorResponse = createErrorResponse("Invalid event ID format");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if event exists
        if (!db_->eventExists(eventId)) {
            json errorResponse = createErrorResponse("Event not found", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if user has access (creator or participant)
        bool isCreator = db_->isEventCreator(eventId, authResult.userId);
        bool isParticipant = db_->isParticipant(eventId, authResult.userId);
        
        if (!isCreator && !isParticipant) {
            json errorResponse = createErrorResponse("Access denied", 403);
            res.status = 403;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Get event details
        json event = db_->getEvent(eventId);
        
        json response = createSuccessResponse();
        response["event"] = event;
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to retrieve event: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void EventsController::updateEvent(const httplib::Request& req, httplib::Response& res) {
    try {
        // Authenticate user
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Extract event ID from URL
        std::string eventId = req.matches[1];
        
        if (!isValidUUID(eventId)) {
            json errorResponse = createErrorResponse("Invalid event ID format");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if event exists
        if (!db_->eventExists(eventId)) {
            json errorResponse = createErrorResponse("Event not found", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if user is the creator
        if (!db_->isEventCreator(eventId, authResult.userId)) {
            json errorResponse = createErrorResponse("Only event creator can update event", 403);
            res.status = 403;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Parse request body
        json requestBody;
        try {
            requestBody = json::parse(req.body);
        } catch (const std::exception& e) {
            json errorResponse = createErrorResponse("Invalid JSON format");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Validate request
        UpdateEventRequest updateReq;
        std::string validationError;
        if (!validateUpdateEventRequest(requestBody, updateReq, validationError)) {
            json errorResponse = createErrorResponse(validationError);
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Build updates object
        json updates;
        if (!updateReq.name.empty()) updates["name"] = updateReq.name;
        if (!updateReq.description.empty()) updates["description"] = updateReq.description;
        if (!updateReq.eventType.empty()) updates["event_type"] = updateReq.eventType;
        if (!updateReq.status.empty()) updates["status"] = updateReq.status;
        if (!updateReq.startDate.empty()) updates["start_date"] = updateReq.startDate;
        if (!updateReq.endDate.empty()) updates["end_date"] = updateReq.endDate;

        // Update event
        json updatedEvent = db_->updateEvent(eventId, updates);
        
        json response = createSuccessResponse();
        response["event"] = updatedEvent;
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to update event: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void EventsController::deleteEvent(const httplib::Request& req, httplib::Response& res) {
    try {
        // Authenticate user
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Extract event ID from URL
        std::string eventId = req.matches[1];
        
        if (!isValidUUID(eventId)) {
            json errorResponse = createErrorResponse("Invalid event ID format");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if event exists
        if (!db_->eventExists(eventId)) {
            json errorResponse = createErrorResponse("Event not found", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if user is the creator
        if (!db_->isEventCreator(eventId, authResult.userId)) {
            json errorResponse = createErrorResponse("Only event creator can delete event", 403);
            res.status = 403;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Delete event
        bool deleted = db_->deleteEvent(eventId);
        
        if (!deleted) {
            json errorResponse = createErrorResponse("Failed to delete event", 500);
            res.status = 500;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }
        
        json response = createSuccessResponse();
        response["message"] = "Event deleted successfully";
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to delete event: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

bool EventsController::validateCreateEventRequest(const json& requestBody, CreateEventRequest& req, std::string& error) {
    // Check required fields
    if (!requestBody.contains("name") || !requestBody["name"].is_string()) {
        error = "Name is required and must be a string";
        return false;
    }
    
    if (!requestBody.contains("description") || !requestBody["description"].is_string()) {
        error = "Description is required and must be a string";
        return false;
    }
    
    if (!requestBody.contains("event_type") || !requestBody["event_type"].is_string()) {
        error = "Event type is required and must be a string";
        return false;
    }

    req.name = trim(requestBody["name"]);
    req.description = trim(requestBody["description"]);
    req.eventType = trim(requestBody["event_type"]);

    // Validate name length
    if (req.name.empty() || req.name.length() > 100) {
        error = "Name must be between 1 and 100 characters";
        return false;
    }

    // Validate event type
    if (!isValidEventType(req.eventType)) {
        error = "Invalid event type";
        return false;
    }

    // Optional fields
    if (requestBody.contains("start_date")) {
        if (!requestBody["start_date"].is_string()) {
            error = "Start date must be a string";
            return false;
        }
        req.startDate = trim(requestBody["start_date"]);
        if (!req.startDate.empty() && !isValidDateFormat(req.startDate)) {
            error = "Invalid start date format (use ISO 8601)";
            return false;
        }
    }

    if (requestBody.contains("end_date")) {
        if (!requestBody["end_date"].is_string()) {
            error = "End date must be a string";
            return false;
        }
        req.endDate = trim(requestBody["end_date"]);
        if (!req.endDate.empty() && !isValidDateFormat(req.endDate)) {
            error = "Invalid end date format (use ISO 8601)";
            return false;
        }
    }

    return true;
}

bool EventsController::validateUpdateEventRequest(const json& requestBody, UpdateEventRequest& req, std::string& error) {
    // All fields are optional for updates
    if (requestBody.contains("name")) {
        if (!requestBody["name"].is_string()) {
            error = "Name must be a string";
            return false;
        }
        req.name = trim(requestBody["name"]);
        if (!req.name.empty() && req.name.length() > 100) {
            error = "Name must be between 1 and 100 characters";
            return false;
        }
    }

    if (requestBody.contains("description")) {
        if (!requestBody["description"].is_string()) {
            error = "Description must be a string";
            return false;
        }
        req.description = trim(requestBody["description"]);
    }

    if (requestBody.contains("event_type")) {
        if (!requestBody["event_type"].is_string()) {
            error = "Event type must be a string";
            return false;
        }
        req.eventType = trim(requestBody["event_type"]);
        if (!req.eventType.empty() && !isValidEventType(req.eventType)) {
            error = "Invalid event type";
            return false;
        }
    }

    if (requestBody.contains("status")) {
        if (!requestBody["status"].is_string()) {
            error = "Status must be a string";
            return false;
        }
        req.status = trim(requestBody["status"]);
        if (!req.status.empty() && !isValidEventStatus(req.status)) {
            error = "Invalid event status";
            return false;
        }
    }

    if (requestBody.contains("start_date")) {
        if (!requestBody["start_date"].is_string()) {
            error = "Start date must be a string";
            return false;
        }
        req.startDate = trim(requestBody["start_date"]);
        if (!req.startDate.empty() && !isValidDateFormat(req.startDate)) {
            error = "Invalid start date format (use ISO 8601)";
            return false;
        }
    }

    if (requestBody.contains("end_date")) {
        if (!requestBody["end_date"].is_string()) {
            error = "End date must be a string";
            return false;
        }
        req.endDate = trim(requestBody["end_date"]);
        if (!req.endDate.empty() && !isValidDateFormat(req.endDate)) {
            error = "Invalid end date format (use ISO 8601)";
            return false;
        }
    }

    return true;
}

bool EventsController::isValidEventType(const std::string& type) {
    static const std::set<std::string> validTypes = {
        "restaurant", "travel", "shared_house", "shopping", 
        "entertainment", "utilities", "other"
    };
    return validTypes.find(type) != validTypes.end();
}

bool EventsController::isValidEventStatus(const std::string& status) {
    static const std::set<std::string> validStatuses = {
        "active", "completed", "cancelled"
    };
    return validStatuses.find(status) != validStatuses.end();
}

bool EventsController::isValidDateFormat(const std::string& date) {
    // Basic ISO 8601 date format validation
    std::regex dateRegex("^\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}(\\.\\d{3})?Z?$");
    return std::regex_match(date, dateRegex);
}

json EventsController::createErrorResponse(const std::string& message, int statusCode) {
    return json{
        {"error", message},
        {"status", statusCode},
        {"timestamp", getCurrentTimestamp()}
    };
}

json EventsController::createSuccessResponse(const json& data) {
    json response = {
        {"success", true},
        {"timestamp", getCurrentTimestamp()}
    };
    
    if (!data.empty()) {
        for (auto& [key, value] : data.items()) {
            response[key] = value;
        }
    }
    
    return response;
}