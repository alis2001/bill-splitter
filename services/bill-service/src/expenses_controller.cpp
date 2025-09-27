#include "expenses_controller.h"
#include "utils.h"
#include <iostream>
#include <regex>
#include <set>

ExpensesController::ExpensesController(std::shared_ptr<Database> db, std::shared_ptr<AuthMiddleware> auth)
    : db_(db), auth_(auth) {}

void ExpensesController::getExpenses(const httplib::Request& req, httplib::Response& res) {
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

        // Get expenses for event
        json expenses = db_->getExpensesByEvent(eventId);
        
        json response = createSuccessResponse();
        response["expenses"] = expenses;
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to retrieve expenses: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void ExpensesController::createExpense(const httplib::Request& req, httplib::Response& res) {
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
        CreateExpenseRequest expenseReq;
        std::string validationError;
        if (!validateCreateExpenseRequest(requestBody, expenseReq, validationError)) {
            json errorResponse = createErrorResponse(validationError);
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if payer exists and has access to event
        if (!db_->userExists(expenseReq.payerId)) {
            json errorResponse = createErrorResponse("Payer not found");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        bool payerIsCreator = db_->isEventCreator(eventId, expenseReq.payerId);
        bool payerIsParticipant = db_->isParticipant(eventId, expenseReq.payerId);
        
        if (!payerIsCreator && !payerIsParticipant) {
            json errorResponse = createErrorResponse("Payer must be event creator or participant");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Create expense
        json expense = db_->createExpense(
            eventId,
            expenseReq.payerId,
            expenseReq.amount,
            expenseReq.description,
            expenseReq.splitType
        );

        json response = createSuccessResponse();
        response["expense"] = expense;
        
        res.status = 201;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to create expense: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void ExpensesController::getExpense(const httplib::Request& req, httplib::Response& res) {
    try {
        // Authenticate user
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Extract event ID and expense ID from URL
        std::string eventId = req.matches[1];
        std::string expenseId = req.matches[2];
        
        if (!isValidUUID(eventId) || !isValidUUID(expenseId)) {
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

        // Check if user has access (creator or participant)
        bool isCreator = db_->isEventCreator(eventId, authResult.userId);
        bool isParticipant = db_->isParticipant(eventId, authResult.userId);
        
        if (!isCreator && !isParticipant) {
            json errorResponse = createErrorResponse("Access denied", 403);
            res.status = 403;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Get expense details
        json expense = db_->getExpense(expenseId);
        
        if (expense.empty()) {
            json errorResponse = createErrorResponse("Expense not found", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Verify expense belongs to this event
        if (expense["event_id"] != eventId) {
            json errorResponse = createErrorResponse("Expense not found", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }
        
        json response = createSuccessResponse();
        response["expense"] = expense;
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to retrieve expense: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void ExpensesController::updateExpense(const httplib::Request& req, httplib::Response& res) {
    try {
        // Authenticate user
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Extract event ID and expense ID from URL
        std::string eventId = req.matches[1];
        std::string expenseId = req.matches[2];
        
        if (!isValidUUID(eventId) || !isValidUUID(expenseId)) {
            json errorResponse = createErrorResponse("Invalid ID format");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Get expense details to check ownership
        json expense = db_->getExpense(expenseId);
        
        if (expense.empty()) {
            json errorResponse = createErrorResponse("Expense not found", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Verify expense belongs to this event
        if (expense["event_id"] != eventId) {
            json errorResponse = createErrorResponse("Expense not found", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if user is the payer or event creator
        bool isCreator = db_->isEventCreator(eventId, authResult.userId);
        bool isPayer = (expense["payer_id"] == authResult.userId);
        
        if (!isCreator && !isPayer) {
            json errorResponse = createErrorResponse("Only expense payer or event creator can update expense", 403);
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
        UpdateExpenseRequest updateReq;
        std::string validationError;
        if (!validateUpdateExpenseRequest(requestBody, updateReq, validationError)) {
            json errorResponse = createErrorResponse(validationError);
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Note: For simplicity, we're not implementing update here
        // In a real implementation, you'd add updateExpense method to Database class
        json errorResponse = createErrorResponse("Expense update not implemented yet", 501);
        res.status = 501;
        res.set_content(errorResponse.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to update expense: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

void ExpensesController::deleteExpense(const httplib::Request& req, httplib::Response& res) {
    try {
        // Authenticate user
        auto authResult = auth_->authenticate(req);
        if (!authResult.success) {
            json errorResponse = auth_->createAuthErrorResponse(authResult.error);
            res.status = 401;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Extract event ID and expense ID from URL
        std::string eventId = req.matches[1];
        std::string expenseId = req.matches[2];
        
        if (!isValidUUID(eventId) || !isValidUUID(expenseId)) {
            json errorResponse = createErrorResponse("Invalid ID format");
            res.status = 400;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Get expense details to check ownership
        json expense = db_->getExpense(expenseId);
        
        if (expense.empty()) {
            json errorResponse = createErrorResponse("Expense not found", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Verify expense belongs to this event
        if (expense["event_id"] != eventId) {
            json errorResponse = createErrorResponse("Expense not found", 404);
            res.status = 404;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Check if user is the payer or event creator
        bool isCreator = db_->isEventCreator(eventId, authResult.userId);
        bool isPayer = (expense["payer_id"] == authResult.userId);
        
        if (!isCreator && !isPayer) {
            json errorResponse = createErrorResponse("Only expense payer or event creator can delete expense", 403);
            res.status = 403;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }

        // Delete expense
        bool deleted = db_->deleteExpense(expenseId);
        
        if (!deleted) {
            json errorResponse = createErrorResponse("Failed to delete expense", 500);
            res.status = 500;
            res.set_content(errorResponse.dump(), "application/json");
            return;
        }
        
        json response = createSuccessResponse();
        response["message"] = "Expense deleted successfully";
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        json errorResponse = createErrorResponse("Failed to delete expense: " + std::string(e.what()), 500);
        res.status = 500;
        res.set_content(errorResponse.dump(), "application/json");
    }
}

bool ExpensesController::validateCreateExpenseRequest(const json& requestBody, CreateExpenseRequest& req, std::string& error) {
    // Check required fields
    if (!requestBody.contains("payer_id") || !requestBody["payer_id"].is_string()) {
        error = "Payer ID is required and must be a string";
        return false;
    }
    
    if (!requestBody.contains("amount") || !requestBody["amount"].is_number()) {
        error = "Amount is required and must be a number";
        return false;
    }
    
    if (!requestBody.contains("description") || !requestBody["description"].is_string()) {
        error = "Description is required and must be a string";
        return false;
    }

    req.payerId = trim(requestBody["payer_id"]);
    req.amount = requestBody["amount"];
    req.description = trim(requestBody["description"]);

    // Validate payer ID format
    if (!isValidUUID(req.payerId)) {
        error = "Invalid payer ID format";
        return false;
    }

    // Validate amount
    if (!isValidAmount(req.amount)) {
        error = "Amount must be positive";
        return false;
    }

    // Validate description length
    if (req.description.empty() || req.description.length() > 255) {
        error = "Description must be between 1 and 255 characters";
        return false;
    }

    // Optional fields
    if (requestBody.contains("split_type")) {
        if (!requestBody["split_type"].is_string()) {
            error = "Split type must be a string";
            return false;
        }
        req.splitType = trim(requestBody["split_type"]);
        if (!req.splitType.empty() && !isValidSplitType(req.splitType)) {
            error = "Invalid split type";
            return false;
        }
    } else {
        req.splitType = "equal";
    }

    if (requestBody.contains("expense_date")) {
        if (!requestBody["expense_date"].is_string()) {
            error = "Expense date must be a string";
            return false;
        }
        req.expenseDate = trim(requestBody["expense_date"]);
        if (!req.expenseDate.empty() && !isValidDateFormat(req.expenseDate)) {
            error = "Invalid expense date format (use ISO 8601)";
            return false;
        }
    }

    return true;
}

bool ExpensesController::validateUpdateExpenseRequest(const json& requestBody, UpdateExpenseRequest& req, std::string& error) {
    // All fields are optional for updates
    if (requestBody.contains("amount")) {
        if (!requestBody["amount"].is_number()) {
            error = "Amount must be a number";
            return false;
        }
        req.amount = requestBody["amount"];
        if (!isValidAmount(req.amount)) {
            error = "Amount must be positive";
            return false;
        }
    }

    if (requestBody.contains("description")) {
        if (!requestBody["description"].is_string()) {
            error = "Description must be a string";
            return false;
        }
        req.description = trim(requestBody["description"]);
        if (!req.description.empty() && req.description.length() > 255) {
            error = "Description must be between 1 and 255 characters";
            return false;
        }
    }

    if (requestBody.contains("split_type")) {
        if (!requestBody["split_type"].is_string()) {
            error = "Split type must be a string";
            return false;
        }
        req.splitType = trim(requestBody["split_type"]);
        if (!req.splitType.empty() && !isValidSplitType(req.splitType)) {
            error = "Invalid split type";
            return false;
        }
    }

    if (requestBody.contains("expense_date")) {
        if (!requestBody["expense_date"].is_string()) {
            error = "Expense date must be a string";
            return false;
        }
        req.expenseDate = trim(requestBody["expense_date"]);
        if (!req.expenseDate.empty() && !isValidDateFormat(req.expenseDate)) {
            error = "Invalid expense date format (use ISO 8601)";
            return false;
        }
    }

    return true;
}

bool ExpensesController::isValidSplitType(const std::string& type) {
    static const std::set<std::string> validTypes = {
        "equal", "percentage", "custom"
    };
    return validTypes.find(type) != validTypes.end();
}

bool ExpensesController::isValidAmount(double amount) {
    return amount > 0.0 && amount <= 999999.99;
}

bool ExpensesController::isValidDateFormat(const std::string& date) {
    // Basic ISO 8601 date format validation
    std::regex dateRegex("^\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}(\\.\\d{3})?Z?$");
    return std::regex_match(date, dateRegex);
}

json ExpensesController::createErrorResponse(const std::string& message, int statusCode) {
    return json{
        {"error", message},
        {"status", statusCode},
        {"timestamp", getCurrentTimestamp()}
    };
}

json ExpensesController::createSuccessResponse(const json& data) {
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