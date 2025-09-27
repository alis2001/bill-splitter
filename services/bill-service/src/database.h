#ifndef DATABASE_H
#define DATABASE_H

#include <pqxx/pqxx>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Database {
public:
    Database();
    ~Database();
    
    bool connect();
    void disconnect();
    
    // Events operations
    json createEvent(const std::string& creatorId, const std::string& name, 
                     const std::string& description, const std::string& eventType,
                     const std::string& startDate = "", const std::string& endDate = "");
    json getEvent(const std::string& eventId);
    json getEventsByUser(const std::string& userId);
    json updateEvent(const std::string& eventId, const json& updates);
    bool deleteEvent(const std::string& eventId);
    
    // Expenses operations
    json createExpense(const std::string& eventId, const std::string& payerId,
                       double amount, const std::string& description,
                       const std::string& splitType = "equal");
    json getExpensesByEvent(const std::string& eventId);
    json getExpense(const std::string& expenseId);
    bool deleteExpense(const std::string& expenseId);
    
    // Participants operations
    json addParticipant(const std::string& eventId, const std::string& userId,
                        double sharePercentage = 0.0, double customAmount = 0.0);
    json getParticipantsByEvent(const std::string& eventId);
    bool removeParticipant(const std::string& eventId, const std::string& userId);
    bool updateParticipant(const std::string& eventId, const std::string& userId,
                           double sharePercentage, double customAmount);
    
    // Utility functions
    bool userExists(const std::string& userId);
    bool eventExists(const std::string& eventId);
    bool isEventCreator(const std::string& eventId, const std::string& userId);
    bool isParticipant(const std::string& eventId, const std::string& userId);

private:
    std::unique_ptr<pqxx::connection> conn_;
    std::string connectionString_;
    
    void initializeConnection();
    json rowToJson(const pqxx::row& row, const std::vector<std::string>& columns);
};

#endif