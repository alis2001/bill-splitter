#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#include <hiredis/hiredis.h>
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class RedisClient {
public:
    RedisClient();
    ~RedisClient();
    
    bool connect();
    void disconnect();
    
    // Token operations
    bool setToken(const std::string& token, const std::string& userData, int ttl = 86400);
    std::string getToken(const std::string& token);
    bool deleteToken(const std::string& token);
    bool tokenExists(const std::string& token);
    
    // Cache operations
    bool setCache(const std::string& key, const std::string& value, int ttl = 3600);
    std::string getCache(const std::string& key);
    bool deleteCache(const std::string& key);
    
    // Generic operations
    bool ping();
    bool isConnected();

private:
    redisContext* context_;
    std::string host_;
    int port_;
    std::string password_;
    
    void initializeConnection();
    bool authenticate();
    void handleReply(redisReply* reply);
    std::string escapeString(const std::string& str);
};

#endif