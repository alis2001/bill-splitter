#include "database.h"
#include "utils.h"
#include <iostream>
#include <stdexcept>

Database::Database() {
    initializeConnection();
}

Database::~Database() {
    disconnect();
}

void Database::initializeConnection() {
    std::string host = getEnvVar("DB_HOST", "postgres");
    std::string port = getEnvVar("DB_PORT", "5432");
    std::string dbname = getEnvVar("DB_NAME", "bill_splitter_db");
    std::string user = getEnvVar("DB_USER", "billsplitter_user");
    std::string password = getEnvVar("DB_PASSWORD", "");
    
    connectionString_ = "host=" + host + " port=" + port + " dbname=" + dbname + 
                       " user=" + user + " password=" + password;
}

bool Database::connect() {
    try {
        conn_ = std::make_unique<pqxx::connection>(connectionString_);
        return conn_->is_open();
    } catch (const std::exception& e) {
        std::cerr << "Database connection failed: " << e.what() << std::endl;
        return false;
    }
}

void Database::disconnect() {
    if (conn_) {
        conn_.reset();  // Just reset the pointer, connection closes automatically
    }
}

json Database::createEvent(const std::string& creatorId, const std::string& name,
                          const std::string& description, const std::string& eventType,
                          const std::string& startDate, const std::string& endDate) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                throw std::runtime_error("Database connection failed");
            }
        }
        
        pqxx::work txn(*conn_);
        
        std::string query = "INSERT INTO events (creator_id, name, description, event_type";
        std::string values = "VALUES ($1, $2, $3, $4";
        
        if (!startDate.empty()) {
            query += ", start_date";
            values += ", $5";
        }
        if (!endDate.empty()) {
            query += ", end_date";
            values += (!startDate.empty() ? ", $6" : ", $5");
        }
        
        query += ") " + values + ") RETURNING id, created_at";
        
        pqxx::result result;
        if (!startDate.empty() && !endDate.empty()) {
            result = txn.exec_params(query, creatorId, name, description, eventType, startDate, endDate);
        } else if (!startDate.empty()) {
            result = txn.exec_params(query, creatorId, name, description, eventType, startDate);
        } else {
            result = txn.exec_params(query, creatorId, name, description, eventType);
        }
        
        txn.commit();
        
        if (result.size() > 0) {
            json event = {
                {"id", result[0][0].c_str()},
                {"creator_id", creatorId},
                {"name", name},
                {"description", description},
                {"event_type", eventType},
                {"status", "active"},
                {"created_at", result[0][1].c_str()}
            };
            
            if (!startDate.empty()) event["start_date"] = startDate;
            if (!endDate.empty()) event["end_date"] = endDate;
            
            return event;
        }
        
        throw std::runtime_error("Failed to create event");
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error: " + std::string(e.what()));
    }
}

json Database::getEvent(const std::string& eventId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                throw std::runtime_error("Database connection failed");
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "SELECT e.id, e.payer_id, e.amount, e.description, e.split_type, e.expense_date, "
            "e.created_at, u.name as payer_name, u.family_name as payer_family_name "
            "FROM expenses e "
            "JOIN users u ON e.payer_id = u.id "
            "WHERE e.event_id = $1 "
            "ORDER BY e.expense_date DESC", eventId
        );
        
        if (result.size() == 0) {
            return json{};
        }
        
        auto row = result[0];
        json event = {
            {"id", row[0].c_str()},
            {"creator_id", row[1].c_str()},
            {"name", row[2].c_str()},
            {"description", row[3].c_str()},
            {"event_type", row[4].c_str()},
            {"status", row[5].c_str()},
            {"created_at", row[8].c_str()},
            {"updated_at", row[9].c_str()},
            {"creator", {
                {"name", row[10].c_str()},
                {"family_name", row[11].c_str()}
            }}
        };
        
        if (!row[6].is_null()) event["start_date"] = row[6].c_str();
        if (!row[7].is_null()) event["end_date"] = row[7].c_str();
        
        return event;
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error: " + std::string(e.what()));
    }
}

json Database::getEventsByUser(const std::string& userId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                throw std::runtime_error("Database connection failed");
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "SELECT DISTINCT e.id, e.name, e.description, e.event_type, e.status, "
            "e.start_date, e.end_date, e.created_at "
            "FROM events e "
            "LEFT JOIN participants p ON e.id = p.event_id "
            "WHERE e.creator_id = $1 OR p.user_id = $1 "
            "ORDER BY e.created_at DESC", userId
        );
        
        json events = json::array();
        
        for (const auto& row : result) {
            json event = {
                {"id", row[0].c_str()},
                {"name", row[1].c_str()},
                {"description", row[2].c_str()},
                {"event_type", row[3].c_str()},
                {"status", row[4].c_str()},
                {"created_at", row[7].c_str()}
            };
            
            if (!row[5].is_null()) event["start_date"] = row[5].c_str();
            if (!row[6].is_null()) event["end_date"] = row[6].c_str();
            
            events.push_back(event);
        }
        
        return events;
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error: " + std::string(e.what()));
    }
}

json Database::updateEvent(const std::string& eventId, const json& updates) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                throw std::runtime_error("Database connection failed");
            }
        }
        
        if (updates.empty()) {
            throw std::runtime_error("No updates provided");
        }
        
        pqxx::work txn(*conn_);
        
        std::string query = "UPDATE events SET ";
        std::vector<std::string> setClauses;
        
        for (auto& [key, value] : updates.items()) {
            std::string clause = key + " = ";
            if (value.is_string()) {
                clause += txn.quote(value.get<std::string>());
            } else if (value.is_null()) {
                clause += "NULL";
            } else {
                clause += txn.quote(value.dump());
            }
            setClauses.push_back(clause);
        }
        
        for (size_t i = 0; i < setClauses.size(); ++i) {
            if (i > 0) query += ", ";
            query += setClauses[i];
        }
        
        query += ", updated_at = CURRENT_TIMESTAMP";
        query += " WHERE id = " + txn.quote(eventId);
        query += " RETURNING id, name, description, event_type, status, start_date, end_date, created_at, updated_at";
        
        pqxx::result result = txn.exec(query);
        txn.commit();
        
        if (result.size() > 0) {
            auto row = result[0];
            json event = {
                {"id", row[0].c_str()},
                {"name", row[1].c_str()},
                {"description", row[2].c_str()},
                {"event_type", row[3].c_str()},
                {"status", row[4].c_str()},
                {"created_at", row[7].c_str()},
                {"updated_at", row[8].c_str()}
            };
            
            if (!row[5].is_null()) event["start_date"] = row[5].c_str();
            if (!row[6].is_null()) event["end_date"] = row[6].c_str();
            
            return event;
        }
        
        throw std::runtime_error("Event not found");
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error: " + std::string(e.what()));
    }
}

bool Database::deleteEvent(const std::string& eventId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                return false;
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "DELETE FROM events WHERE id = $1", eventId
        );
        
        txn.commit();
        
        return result.affected_rows() > 0;
        
    } catch (const std::exception& e) {
        return false;
    }
}

json Database::createExpense(const std::string& eventId, const std::string& payerId,
                            double amount, const std::string& description,
                            const std::string& splitType) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                throw std::runtime_error("Database connection failed");
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "INSERT INTO expenses (event_id, payer_id, amount, description, split_type) "
            "VALUES ($1, $2, $3, $4, $5) "
            "RETURNING id, expense_date, created_at",
            eventId, payerId, amount, description, splitType
        );
        
        txn.commit();
        
        if (result.size() > 0) {
            return json{
                {"id", result[0][0].c_str()},
                {"event_id", eventId},
                {"payer_id", payerId},
                {"amount", amount},
                {"description", description},
                {"split_type", splitType},
                {"expense_date", result[0][1].c_str()},
                {"created_at", result[0][2].c_str()}
            };
        }
        
        throw std::runtime_error("Failed to create expense");
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error: " + std::string(e.what()));
    }
}

json Database::getExpensesByEvent(const std::string& eventId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                throw std::runtime_error("Database connection failed");
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "SELECT e.id, e.payer_id, e.amount, e.description, e.split_type, e.expense_date, "
            "e.created_at, u.name as payer_name, u.family_name as payer_family_name "
            "FROM expenses e "
            "JOIN users u ON e.payer_id = u.id "
            "WHERE e.event_id = $1 "
            "ORDER BY e.expense_date DESC", eventId
        );
        
        json expenses = json::array();
        
        for (const auto& row : result) {
            json expense = {
                {"id", row[0].c_str()},
                {"payer_id", row[1].c_str()},         // NEW: payer_id
                {"amount", row[2].as<double>()},      // FIXED: was row[1], now row[2]
                {"description", row[3].c_str()},      // FIXED: was row[2], now row[3]
                {"split_type", row[4].c_str()},       // FIXED: was row[3], now row[4]
                {"expense_date", row[5].c_str()},     // FIXED: was row[4], now row[5]
                {"created_at", row[6].c_str()},       // FIXED: was row[5], now row[6]
                {"payer", {
                    {"name", row[7].c_str()},         // FIXED: was row[6], now row[7]
                    {"family_name", row[8].c_str()}   // FIXED: was row[7], now row[8]
                }}
            };
            
            expenses.push_back(expense);
        }
        
        return expenses;
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error: " + std::string(e.what()));
    }
}

json Database::getExpense(const std::string& expenseId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                throw std::runtime_error("Database connection failed");
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "SELECT e.id, e.event_id, e.payer_id, e.amount, e.description, "
            "e.split_type, e.expense_date, e.created_at, "
            "u.name as payer_name, u.family_name as payer_family_name "
            "FROM expenses e "
            "JOIN users u ON e.payer_id = u.id "
            "WHERE e.id = $1", expenseId
        );
        
        if (result.size() == 0) {
            return json{};
        }
        
        auto row = result[0];
        return json{
            {"id", row[0].c_str()},
            {"event_id", row[1].c_str()},
            {"payer_id", row[2].c_str()},
            {"amount", row[3].as<double>()},
            {"description", row[4].c_str()},
            {"split_type", row[5].c_str()},
            {"expense_date", row[6].c_str()},
            {"created_at", row[7].c_str()},
            {"payer", {
                {"name", row[8].c_str()},
                {"family_name", row[9].c_str()}
            }}
        };
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error: " + std::string(e.what()));
    }
}

bool Database::deleteExpense(const std::string& expenseId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                return false;
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "DELETE FROM expenses WHERE id = $1", expenseId
        );
        
        txn.commit();
        
        return result.affected_rows() > 0;
        
    } catch (const std::exception& e) {
        return false;
    }
}

json Database::addParticipant(const std::string& eventId, const std::string& userId,
                             double sharePercentage, double customAmount) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                throw std::runtime_error("Database connection failed");
            }
        }
        
        pqxx::work txn(*conn_);
        
        std::string query = "INSERT INTO participants (event_id, user_id";
        std::string values = "VALUES (" + txn.quote(eventId) + ", " + txn.quote(userId);
        
        if (sharePercentage > 0) {
            query += ", share_percentage";
            values += ", " + std::to_string(sharePercentage);
        }
        
        if (customAmount > 0) {
            query += ", custom_amount";
            values += ", " + std::to_string(customAmount);
        }
        
        query += ") " + values + ") RETURNING id, joined_at";
        
        pqxx::result result = txn.exec(query);
        txn.commit();
        
        if (result.size() > 0) {
            return json{
                {"id", result[0][0].c_str()},
                {"event_id", eventId},
                {"user_id", userId},
                {"status", "active"},
                {"joined_at", result[0][1].c_str()}
            };
        }
        
        throw std::runtime_error("Failed to add participant");
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error: " + std::string(e.what()));
    }
}

json Database::getParticipantsByEvent(const std::string& eventId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                throw std::runtime_error("Database connection failed");
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "SELECT p.id, p.user_id, p.share_percentage, p.custom_amount, "
            "p.status, p.joined_at, u.name, u.family_name, u.email "
            "FROM participants p "
            "JOIN users u ON p.user_id = u.id "
            "WHERE p.event_id = $1 AND p.status = 'active' "
            "ORDER BY p.joined_at", eventId
        );
        
        json participants = json::array();
        
        for (const auto& row : result) {
            json participant = {
                {"id", row[0].c_str()},
                {"user_id", row[1].c_str()},
                {"status", row[4].c_str()},
                {"joined_at", row[5].c_str()},
                {"user", {
                    {"name", row[6].c_str()},
                    {"family_name", row[7].c_str()},
                    {"email", row[8].c_str()}
                }}
            };
            
            if (!row[2].is_null()) participant["share_percentage"] = row[2].as<double>();
            if (!row[3].is_null()) participant["custom_amount"] = row[3].as<double>();
            
            participants.push_back(participant);
        }
        
        return participants;
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Database error: " + std::string(e.what()));
    }
}

bool Database::removeParticipant(const std::string& eventId, const std::string& userId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                return false;
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "DELETE FROM participants WHERE event_id = $1 AND user_id = $2", 
            eventId, userId
        );
        
        txn.commit();
        
        return result.affected_rows() > 0;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool Database::updateParticipant(const std::string& eventId, const std::string& userId,
                                double sharePercentage, double customAmount) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                return false;
            }
        }
        
        pqxx::work txn(*conn_);
        
        std::string query = "UPDATE participants SET ";
        
        if (sharePercentage > 0) {
            query += "share_percentage = " + std::to_string(sharePercentage);
        } else {
            query += "share_percentage = NULL";
        }
        
        if (customAmount > 0) {
            query += ", custom_amount = " + std::to_string(customAmount);
        } else {
            query += ", custom_amount = NULL";
        }
        
        query += ", updated_at = CURRENT_TIMESTAMP";
        query += " WHERE event_id = " + txn.quote(eventId);
        query += " AND user_id = " + txn.quote(userId);
        
        pqxx::result result = txn.exec(query);
        txn.commit();
        
        return result.affected_rows() > 0;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool Database::userExists(const std::string& userId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                return false;
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "SELECT 1 FROM users WHERE id = $1 AND is_active = true", userId
        );
        
        return result.size() > 0;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool Database::eventExists(const std::string& eventId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                return false;
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "SELECT 1 FROM events WHERE id = $1", eventId
        );
        
        return result.size() > 0;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool Database::isEventCreator(const std::string& eventId, const std::string& userId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                return false;
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "SELECT 1 FROM events WHERE id = $1 AND creator_id = $2", eventId, userId
        );
        
        return result.size() > 0;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool Database::isParticipant(const std::string& eventId, const std::string& userId) {
    try {
        if (!conn_ || !conn_->is_open()) {
            if (!connect()) {
                return false;
            }
        }
        
        pqxx::work txn(*conn_);
        
        pqxx::result result = txn.exec_params(
            "SELECT 1 FROM participants WHERE event_id = $1 AND user_id = $2 AND status = 'active'", 
            eventId, userId
        );
        
        return result.size() > 0;
        
    } catch (const std::exception& e) {
        return false;
    }
}