#ifndef SPLIT_CALCULATOR_H
#define SPLIT_CALCULATOR_H

#include <vector>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ExpenseShare {
    std::string userId;
    double amount;
    double percentage;
};

struct Settlement {
    std::string fromUserId;
    std::string toUserId;
    double amount;
};

class SplitCalculator {
public:
    // Calculate individual shares for an expense
    static std::vector<ExpenseShare> calculateExpenseShares(
        double totalAmount,
        const std::string& splitType,
        const std::vector<std::string>& participantIds,
        const json& customShares = json::object()
    );
    
    // Calculate who owes whom for an entire event
    static std::vector<Settlement> calculateEventSettlements(
        const json& expenses,
        const json& participants
    );
    
    // Get balance summary for each user
    static json calculateUserBalances(
        const std::string& eventId,
        const json& expenses,
        const json& participants
    );

private:
    static std::vector<Settlement> optimizeSettlements(const std::map<std::string, double>& balances);
};

#endif