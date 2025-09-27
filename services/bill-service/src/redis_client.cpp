#include "redis_client.h"
#include "utils.h"
#include <iostream>
#include <cstring>

RedisClient::RedisClient() : context_(nullptr) {
    initializeConnection();
}

RedisClient::~RedisClient() {
    disconnect();
}

void RedisClient::initializeConnection() {
    host_ = getEnvVar("REDIS_HOST", "redis");
    port_ = std::stoi(getEnvVar("REDIS_PORT", "6379"));
    password_ = getEnvVar("REDIS_PASSWORD", "");
}

bool RedisClient::connect() {
    try {
        if (context_) {
            disconnect();
        }
        
        struct timeval timeout = { 1, 500000 }; // 1.5 seconds
        context_ = redisConnectWithTimeout(host_.c_str(), port_, timeout);
        
        if (context_ == nullptr || context_->err) {
            if (context_) {
                std::cerr << "Redis connection error: " << context_->errstr << std::endl;
                redisFree(context_);
                context_ = nullptr;
            } else {
                std::cerr << "Redis connection error: can't allocate redis context" << std::endl;
            }
            return false;
        }
        
        if (!password_.empty()) {
            if (!authenticate()) {
                disconnect();
                return false;
            }
        }
        
        return ping();
        
    } catch (const std::exception& e) {
        std::cerr << "Redis connection exception: " << e.what() << std::endl;
        return false;
    }
}

void RedisClient::disconnect() {
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
    }
}

bool RedisClient::authenticate() {
    if (!context_ || password_.empty()) {
        return true;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "AUTH %s", password_.c_str());
    
    if (!reply) {
        return false;
    }
    
    bool success = (reply->type == REDIS_REPLY_STATUS && 
                   strcmp(reply->str, "OK") == 0);
    
    freeReplyObject(reply);
    return success;
}

bool RedisClient::ping() {
    if (!context_) {
        return false;
    }
    
    redisReply* reply = (redisReply*)redisCommand(context_, "PING");
    
    if (!reply) {
        return false;
    }
    
    bool success = (reply->type == REDIS_REPLY_STATUS && 
                   strcmp(reply->str, "PONG") == 0);
    
    freeReplyObject(reply);
    return success;
}

bool RedisClient::isConnected() {
    return context_ != nullptr && context_->err == 0;
}

bool RedisClient::setToken(const std::string& token, const std::string& userData, int ttl) {
    if (!isConnected()) {
        if (!connect()) {
            return false;
        }
    }
    
    std::string key = "token:" + token;
    std::string escapedData = escapeString(userData);
    
    redisReply* reply = (redisReply*)redisCommand(context_, 
        "SETEX %s %d %s", key.c_str(), ttl, escapedData.c_str());
    
    if (!reply) {
        return false;
    }
    
    bool success = (reply->type == REDIS_REPLY_STATUS && 
                   strcmp(reply->str, "OK") == 0);
    
    freeReplyObject(reply);
    return success;
}

std::string RedisClient::getToken(const std::string& token) {
    if (!isConnected()) {
        if (!connect()) {
            return "";
        }
    }
    
    std::string key = "token:" + token;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "GET %s", key.c_str());
    
    if (!reply) {
        return "";
    }
    
    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }
    
    freeReplyObject(reply);
    return result;
}

bool RedisClient::deleteToken(const std::string& token) {
    if (!isConnected()) {
        if (!connect()) {
            return false;
        }
    }
    
    std::string key = "token:" + token;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "DEL %s", key.c_str());
    
    if (!reply) {
        return false;
    }
    
    bool success = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    
    freeReplyObject(reply);
    return success;
}

bool RedisClient::tokenExists(const std::string& token) {
    if (!isConnected()) {
        if (!connect()) {
            return false;
        }
    }
    
    std::string key = "token:" + token;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "EXISTS %s", key.c_str());
    
    if (!reply) {
        return false;
    }
    
    bool exists = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    
    freeReplyObject(reply);
    return exists;
}

bool RedisClient::setCache(const std::string& key, const std::string& value, int ttl) {
    if (!isConnected()) {
        if (!connect()) {
            return false;
        }
    }
    
    std::string cacheKey = "cache:" + key;
    std::string escapedValue = escapeString(value);
    
    redisReply* reply = (redisReply*)redisCommand(context_, 
        "SETEX %s %d %s", cacheKey.c_str(), ttl, escapedValue.c_str());
    
    if (!reply) {
        return false;
    }
    
    bool success = (reply->type == REDIS_REPLY_STATUS && 
                   strcmp(reply->str, "OK") == 0);
    
    freeReplyObject(reply);
    return success;
}

std::string RedisClient::getCache(const std::string& key) {
    if (!isConnected()) {
        if (!connect()) {
            return "";
        }
    }
    
    std::string cacheKey = "cache:" + key;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "GET %s", cacheKey.c_str());
    
    if (!reply) {
        return "";
    }
    
    std::string result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }
    
    freeReplyObject(reply);
    return result;
}

bool RedisClient::deleteCache(const std::string& key) {
    if (!isConnected()) {
        if (!connect()) {
            return false;
        }
    }
    
    std::string cacheKey = "cache:" + key;
    
    redisReply* reply = (redisReply*)redisCommand(context_, "DEL %s", cacheKey.c_str());
    
    if (!reply) {
        return false;
    }
    
    bool success = reply->type == REDIS_REPLY_INTEGER && reply->integer > 0;
    
    freeReplyObject(reply);
    return success;
}

void RedisClient::handleReply(redisReply* reply) {
    if (!reply) {
        std::cerr << "Redis: NULL reply" << std::endl;
        return;
    }
    
    if (reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "Redis error: " << reply->str << std::endl;
    }
}

std::string RedisClient::escapeString(const std::string& str) {
    // Basic escaping for Redis strings
    std::string escaped = str;
    
    // Replace any problematic characters
    size_t pos = 0;
    while ((pos = escaped.find("\"", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    return escaped;
}