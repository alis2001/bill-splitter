#include "split_calculator.h"
#include <algorithm>
#include <cmath>

std::vector<ExpenseShare> SplitCalculator::calculateExpenseShares(
    double totalAmount,
    const std::string& splitType,
    const std::vector<std::string>& participantIds,
    const json& customShares) {
    
    std::vector<ExpenseShare> shares;
    
    if (splitType == "equal") {
        double shareAmount = totalAmount / participantIds.size();
        for (const auto& userId : participantIds) {
            shares.push_back({userId, shareAmount, 100.0 / participantIds.size()});
        }
    }
    else if (splitType == "percentage" && !customShares.empty()) {
        for (const auto& userId : participantIds) {
            if (customShares.contains(userId)) {
                double percentage = customShares[userId];
                double amount = (totalAmount * percentage) / 100.0;
                shares.push_back({userId, amount, percentage});
            }
        }
    }
    else if (splitType == "custom" && !customShares.empty()) {
        for (const auto& userId : participantIds) {
            if (customShares.contains(userId)) {
                double amount = customShares[userId];
                double percentage = (amount / totalAmount) * 100.0;
                shares.push_back({userId, amount, percentage});
            }
        }
    }
    
    return shares;
}

std::vector<Settlement> SplitCalculator::calculateEventSettlements(
    const json& expenses,
    const json& participants) {
    
    std::map<std::string, double> balances;
    
    // Initialize balances - check if participants is array
    if (participants.is_array()) {
        for (const auto& participant : participants) {
            if (participant.contains("user_id")) {
                balances[participant["user_id"]] = 0.0;
            }
        }
    }
    
    // Calculate balances from expenses - check if expenses is array
    if (expenses.is_array()) {
        for (const auto& expense : expenses) {
            if (!expense.contains("payer_id") || !expense.contains("amount")) {
                continue; // Skip invalid expenses
            }
            
            std::string payerId = expense["payer_id"];
            double amount = expense["amount"];
            std::string splitType = expense.contains("split_type") ? expense["split_type"] : "equal";
            
            // Get participant IDs
            std::vector<std::string> participantIds;
            if (participants.is_array()) {
                for (const auto& participant : participants) {
                    if (participant.contains("user_id")) {
                        participantIds.push_back(participant["user_id"]);
                    }
                }
            }
            
            if (participantIds.empty()) {
                continue; // Skip if no participants
            }
            
            // Calculate shares
            auto shares = calculateExpenseShares(amount, splitType, participantIds);
            
            // Payer gets credit for paying
            if (balances.find(payerId) != balances.end()) {
                balances[payerId] += amount;
            }
            
            // Everyone owes their share
            for (const auto& share : shares) {
                if (balances.find(share.userId) != balances.end()) {
                    balances[share.userId] -= share.amount;
                }
            }
        }
    }
    
    return optimizeSettlements(balances);
}

json SplitCalculator::calculateUserBalances(
    const std::string& eventId,
    const json& expenses,
    const json& participants) {
    
    json result = json::object();
    std::map<std::string, double> balances;
    
    // Initialize balances
    if (participants.is_array()) {
        for (const auto& participant : participants) {
            if (participant.contains("user_id")) {
                std::string userId = participant["user_id"];
                balances[userId] = 0.0;
            }
        }
    }
    
    // Calculate from expenses  
    if (expenses.is_array()) {
        for (const auto& expense : expenses) {
            if (!expense.contains("payer_id") || !expense.contains("amount")) {
                continue;
            }
            
            std::string payerId = expense["payer_id"];
            double amount = expense["amount"];
            std::string splitType = expense.contains("split_type") ? expense["split_type"] : "equal";
            
            std::vector<std::string> participantIds;
            if (participants.is_array()) {
                for (const auto& participant : participants) {
                    if (participant.contains("user_id")) {
                        participantIds.push_back(participant["user_id"]);
                    }
                }
            }
            
            if (participantIds.empty()) {
                continue;
            }
            
            auto shares = calculateExpenseShares(amount, splitType, participantIds);
            
            if (balances.find(payerId) != balances.end()) {
                balances[payerId] += amount;
            }
            
            for (const auto& share : shares) {
                if (balances.find(share.userId) != balances.end()) {
                    balances[share.userId] -= share.amount;
                }
            }
        }
    }
    
    // Convert to JSON
    for (const auto& [userId, balance] : balances) {
        result[userId] = balance;
    }
    
    return result;
}

std::vector<Settlement> SplitCalculator::optimizeSettlements(const std::map<std::string, double>& balances) {
    std::vector<Settlement> settlements;
    std::vector<std::pair<std::string, double>> debtors;
    std::vector<std::pair<std::string, double>> creditors;
    
    // Separate debtors and creditors
    for (const auto& [userId, balance] : balances) {
        if (balance < -0.01) {  // Owes money
            debtors.push_back({userId, -balance});
        } else if (balance > 0.01) {  // Is owed money
            creditors.push_back({userId, balance});
        }
    }
    
    // Sort by amount
    std::sort(debtors.begin(), debtors.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    std::sort(creditors.begin(), creditors.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Match debtors with creditors
    size_t i = 0, j = 0;
    while (i < debtors.size() && j < creditors.size()) {
        double amount = std::min(debtors[i].second, creditors[j].second);
        
        settlements.push_back({
            debtors[i].first,   // from
            creditors[j].first, // to  
            amount
        });
        
        debtors[i].second -= amount;
        creditors[j].second -= amount;
        
        if (debtors[i].second < 0.01) i++;
        if (creditors[j].second < 0.01) j++;
    }
    
    return settlements;
}