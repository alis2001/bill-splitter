#ifndef AUTH_MIDDLEWARE_H
#define AUTH_MIDDLEWARE_H

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <httplib.h>
#include "redis_client.h"

using json = nlohmann::json;

class AuthMiddleware {
public:
    explicit AuthMiddleware(std::shared_ptr<RedisClient> redis);
    
    struct AuthResult {
        bool success;
        std::string userId;
        std::string email;
        std::string error;
    };
    
    AuthResult authenticate(const httplib::Request& req);
    bool requireAuth(const httplib::Request& req, httplib::Response& res);
    
    static std::string extractToken(const std::string& authHeader);
    static json createAuthErrorResponse(const std::string& message, int statusCode = 401);

private:
    std::shared_ptr<RedisClient> redis_;
    std::string jwtSecret_;
    
    bool verifyJWT(const std::string& token, std::string& userId, std::string& email);
    json parseJWTPayload(const std::string& token);
    std::string base64Decode(const std::string& input);
    bool isValidJWTStructure(const std::string& token);
};

#endif