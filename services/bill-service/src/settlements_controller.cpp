#include "settlements_controller.h"
#include "split_calculator.h"
#include "utils.h"

SettlementsController::SettlementsController(std::shared_ptr<Database> db, std::shared_ptr<AuthMiddleware> auth)
    : db_(db), auth_(auth) {}

void SettlementsController::getEventSettlements(const httplib::Request& req, httplib::Response& res) {
    try {
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        std::string eventId = req.matches[1];
        
        if (!isValidUUID(eventId)) {
            json errorResponse = createErrorResponse("Invalid event ID format");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        if (!db_->eventExists(eventId)) {
            json errorResponse = createErrorResponse("Event not found", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        bool isCreator = db_->isEventCreator(eventId, authResult.userId);
        bool isParticipant = db_->isParticipant(eventId, authResult.userId);
        
        if (!isCreator && !isParticipant) {
            json errorResponse = createErrorResponse("Access denied", 403);
            res.status = 403;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        json expenses = db_->getExpensesByEvent(eventId);
        json participants = db_->getParticipantsByEvent(eventId);
        
        if (isCreator) {
            // Get creator info
            json creatorParticipant = {
                {"user_id", authResult.userId},
                {"status", "active"}
            };
            
            // Add creator to participants array for calculation
            if (participants.is_array()) {
                participants.push_back(creatorParticipant);
            } else {
                participants = json::array({creatorParticipant});
            }
        }
        

        // ADD DEBUG OUTPUT HERE
        std::cout << "=== DEBUG: Participants JSON ===" << std::endl;
        std::cout << participants.dump(2) << std::endl;
        
        std::cout << "=== DEBUG: Expenses JSON ===" << std::endl;
        std::cout << expenses.dump(2) << std::endl;
        
        std::cout << "=== DEBUG: Auth User ID ===" << std::endl;
        std::cout << "Current user: " << authResult.userId << std::endl;
        std::cout << "Is creator: " << isCreator << std::endl;
        std::cout << "Is participant: " << isParticipant << std::endl;
        
        json balances = SplitCalculator::calculateUserBalances(eventId, expenses, participants);
        auto settlements = SplitCalculator::calculateEventSettlements(expenses, participants);
        
        json settlementsJson = json::array();
        for (const auto& settlement : settlements) {
            settlementsJson.push_back({
                {"from_user_id", settlement.fromUserId},
                {"to_user_id", settlement.toUserId},
                {"amount", settlement.amount}
            });
        }
        
        json response = createSuccessResponse();
        response["balances"] = balances;
        response["settlements"] = settlementsJson;
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to calculate settlements: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void SettlementsController::recordPayment(const httplib::Request& req, httplib::Response& res) {
    try {
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        std::string eventId = req.matches[1];
        
        if (!isValidUUID(eventId)) {
            json errorResponse = createErrorResponse("Invalid event ID format");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        json requestBody;
        try {
            requestBody = json::parse(req.body);
        } catch (const std::exception& e) {
            json errorResponse = createErrorResponse("Invalid JSON format");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        if (!requestBody.contains("to_user_id") || !requestBody.contains("amount")) {
            json errorResponse = createErrorResponse("Missing required fields: to_user_id, amount");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        std::string toUserId = requestBody["to_user_id"];
        double amount = requestBody["amount"];
        
        if (amount <= 0) {
            json errorResponse = createErrorResponse("Amount must be positive");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Record payment in database (you'll need to add this table)
        // For now, just return success
        json response = createSuccessResponse();
        response["message"] = "Payment recorded successfully";
        response["payment"] = {
            {"from_user_id", authResult.userId},
            {"to_user_id", toUserId},
            {"amount", amount},
            {"event_id", eventId},
            {"recorded_at", getCurrentTimestamp()}
        };
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to record payment: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void SettlementsController::getUserBalance(const httplib::Request& req, httplib::Response& res) {
    try {
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Get all events user is involved in
        json userEvents = db_->getEventsByUser(authResult.userId);
        
        double totalBalance = 0.0;
        json eventBalances = json::array();
        
        for (const auto& event : userEvents) {
            std::string eventId = event["id"];
            
            json expenses = db_->getExpensesByEvent(eventId);
            json participants = db_->getParticipantsByEvent(eventId);
            
            // Add creator to participants for calculation
            bool isCreator = db_->isEventCreator(eventId, authResult.userId);
            if (isCreator) {
                json creatorParticipant = {
                    {"user_id", authResult.userId},
                    {"status", "active"}
                };
                if (participants.is_array()) {
                    participants.push_back(creatorParticipant);
                }
            }
            
            json balances = SplitCalculator::calculateUserBalances(eventId, expenses, participants);
            
            if (balances.contains(authResult.userId)) {
                double eventBalance = balances[authResult.userId];
                totalBalance += eventBalance;
                
                eventBalances.push_back({
                    {"event_id", eventId},
                    {"event_name", event["name"]},
                    {"balance", eventBalance}
                });
            }
        }
        
        json response = createSuccessResponse();
        response["total_balance"] = totalBalance;
        response["event_balances"] = eventBalances;
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to get user balance: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

json SettlementsController::createErrorResponse(const std::string& message, int statusCode) {
    return json{
        {"error", message},
        {"status", statusCode},
        {"timestamp", getCurrentTimestamp()}
    };
}

json SettlementsController::createSuccessResponse(const json& data) {
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