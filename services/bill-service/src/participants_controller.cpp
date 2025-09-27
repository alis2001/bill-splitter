#include "participants_controller.h"
#include "utils.h"
#include <iostream>

ParticipantsController::ParticipantsController(std::shared_ptr<Database> db, std::shared_ptr<AuthMiddleware> auth)
    : db_(db), auth_(auth) {}

void ParticipantsController::getParticipants(const httplib::Request& req, httplib::Response& res) {
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

        // Get participants for event
        json participants = db_->getParticipantsByEvent(eventId);
        
        json response = createSuccessResponse();
        response["participants"] = participants;
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to retrieve participants: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void ParticipantsController::addParticipant(const httplib::Request& req, httplib::Response& res) {
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

        // Check if user is the event creator
        if (!db_->isEventCreator(eventId, authResult.userId)) {
            json errorResponse = createErrorResponse("Only event creator can add participants", 403);
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
        AddParticipantRequest participantReq;
        std::string validationError;
        if (!validateAddParticipantRequest(requestBody, participantReq, validationError)) {
            json errorResponse = createErrorResponse(validationError);
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if user to be added exists
        if (!db_->userExists(participantReq.userId)) {
            json errorResponse = createErrorResponse("User not found");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if user is already a participant
        if (db_->isParticipant(eventId, participantReq.userId)) {
            json errorResponse = createErrorResponse("User is already a participant");
            res.status = 409;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if user is the event creator (creator is automatically a participant)
        if (db_->isEventCreator(eventId, participantReq.userId)) {
            json errorResponse = createErrorResponse("Event creator is automatically a participant");
            res.status = 409;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Add participant
        json participant = db_->addParticipant(
            eventId,
            participantReq.userId,
            participantReq.sharePercentage,
            participantReq.customAmount
        );

        json response = createSuccessResponse();
        response["participant"] = participant;
        
        res.status = 201;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to add participant: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void ParticipantsController::updateParticipant(const httplib::Request& req, httplib::Response& res) {
    try {
        // Authenticate user
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Extract event ID and user ID from URL
        std::string eventId = req.matches[1];
        std::string userId = req.matches[2];
        
        if (!isValidUUID(eventId) || !isValidUUID(userId)) {
            json errorResponse = createErrorResponse("Invalid ID format");
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

        // Check if user is participant
        if (!db_->isParticipant(eventId, userId)) {
            json errorResponse = createErrorResponse("User is not a participant", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if current user is the event creator or the participant being updated
        bool isCreator = db_->isEventCreator(eventId, authResult.userId);
        bool isSelf = (authResult.userId == userId);
        
        if (!isCreator && !isSelf) {
            json errorResponse = createErrorResponse("Only event creator or the participant can update participation", 403);
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
        UpdateParticipantRequest updateReq;
        std::string validationError;
        if (!validateUpdateParticipantRequest(requestBody, updateReq, validationError)) {
            json errorResponse = createErrorResponse(validationError);
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Update participant
        bool updated = db_->updateParticipant(
            eventId,
            userId,
            updateReq.sharePercentage,
            updateReq.customAmount
        );

        if (!updated) {
            json errorResponse = createErrorResponse("Failed to update participant", 500);
            res.status = 500;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }
        
        json response = createSuccessResponse();
        response["message"] = "Participant updated successfully";
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to update participant: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void ParticipantsController::removeParticipant(const httplib::Request& req, httplib::Response& res) {
    try {
        // Authenticate user
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Extract event ID and user ID from URL
        std::string eventId = req.matches[1];
        std::string userId = req.matches[2];
        
        if (!isValidUUID(eventId) || !isValidUUID(userId)) {
            json errorResponse = createErrorResponse("Invalid ID format");
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

        // Check if user is participant
        if (!db_->isParticipant(eventId, userId)) {
            json errorResponse = createErrorResponse("User is not a participant", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if current user is the event creator or the participant being removed
        bool isCreator = db_->isEventCreator(eventId, authResult.userId);
        bool isSelf = (authResult.userId == userId);
        
        if (!isCreator && !isSelf) {
            json errorResponse = createErrorResponse("Only event creator or the participant can remove participation", 403);
            res.status = 403;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Prevent removing event creator
        if (db_->isEventCreator(eventId, userId)) {
            json errorResponse = createErrorResponse("Cannot remove event creator from participants");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Remove participant
        bool removed = db_->removeParticipant(eventId, userId);

        if (!removed) {
            json errorResponse = createErrorResponse("Failed to remove participant", 500);
            res.status = 500;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }
        
        json response = createSuccessResponse();
        response["message"] = "Participant removed successfully";
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to remove participant: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

bool ParticipantsController::validateAddParticipantRequest(const json& requestBody, AddParticipantRequest& req, std::string& error) {
    // Check required fields
    if (!requestBody.contains("user_id") || !requestBody["user_id"].is_string()) {
        error = "User ID is required and must be a string";
        return false;
    }

    req.userId = trim(requestBody["user_id"]);

    // Validate user ID format
    if (!isValidUUID(req.userId)) {
        error = "Invalid user ID format";
        return false;
    }

    // Optional fields
    req.sharePercentage = 0.0;
    req.customAmount = 0.0;

    if (requestBody.contains("share_percentage")) {
        if (!requestBody["share_percentage"].is_number()) {
            error = "Share percentage must be a number";
            return false;
        }
        req.sharePercentage = requestBody["share_percentage"];
        if (!isValidPercentage(req.sharePercentage)) {
            error = "Share percentage must be between 0 and 100";
            return false;
        }
    }

    if (requestBody.contains("custom_amount")) {
        if (!requestBody["custom_amount"].is_number()) {
            error = "Custom amount must be a number";
            return false;
        }
        req.customAmount = requestBody["custom_amount"];
        if (!isValidAmount(req.customAmount)) {
            error = "Custom amount must be positive";
            return false;
        }
    }

    return true;
}

bool ParticipantsController::validateUpdateParticipantRequest(const json& requestBody, UpdateParticipantRequest& req, std::string& error) {
    req.sharePercentage = 0.0;
    req.customAmount = 0.0;

    // All fields are optional for updates
    if (requestBody.contains("share_percentage")) {
        if (!requestBody["share_percentage"].is_number()) {
            error = "Share percentage must be a number";
            return false;
        }
        req.sharePercentage = requestBody["share_percentage"];
        if (!isValidPercentage(req.sharePercentage)) {
            error = "Share percentage must be between 0 and 100";
            return false;
        }
    }

    if (requestBody.contains("custom_amount")) {
        if (!requestBody["custom_amount"].is_number()) {
            error = "Custom amount must be a number";
            return false;
        }
        req.customAmount = requestBody["custom_amount"];
        if (!isValidAmount(req.customAmount)) {
            error = "Custom amount must be positive";
            return false;
        }
    }

    // At least one field should be provided
    if (req.sharePercentage == 0.0 && req.customAmount == 0.0) {
        error = "At least one of share_percentage or custom_amount must be provided";
        return false;
    }

    return true;
}

bool ParticipantsController::isValidPercentage(double percentage) {
    return percentage >= 0.0 && percentage <= 100.0;
}

bool ParticipantsController::isValidAmount(double amount) {
    return amount >= 0.0 && amount <= 999999.99;
}

json ParticipantsController::createErrorResponse(const std::string& message, int statusCode) {
    return json{
        {"error", message},
        {"status", statusCode},
        {"timestamp", getCurrentTimestamp()}
    };
}

json ParticipantsController::createSuccessResponse(const json& data) {
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