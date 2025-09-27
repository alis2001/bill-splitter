#include "auth_middleware.h"
#include "utils.h"
#include <jwt-cpp/jwt.h>
#include <iostream>
#include <regex>
#include <algorithm>

AuthMiddleware::AuthMiddleware(std::shared_ptr<RedisClient> redis) 
    : redis_(redis) {
    jwtSecret_ = getEnvVar("JWT_SECRET", "your_super_secure_jwt_secret_key_min_32_chars");
}

AuthMiddleware::AuthResult AuthMiddleware::authenticate(const httplib::Request& req) {
    AuthResult result;
    result.success = false;
    
    // Extract Authorization header
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty()) {
        result.error = "Missing Authorization header";
        return result;
    }
    
    // Extract token
    std::string token = extractToken(authHeader);
    if (token.empty()) {
        result.error = "Invalid Authorization header format";
        return result;
    }
    
    // Verify JWT structure
    if (!isValidJWTStructure(token)) {
        result.error = "Invalid token format";
        return result;
    }
    
    // Check token in Redis
    if (!redis_->tokenExists(token)) {
        result.error = "Token expired or invalid";
        return result;
    }
    
    // Get token data from Redis
    std::string tokenData = redis_->getToken(token);
    if (tokenData.empty()) {
        result.error = "Token not found";
        return result;
    }
    
    try {
        // Parse token data from Redis
        json cachedData = json::parse(tokenData);
        
        // Verify JWT signature
        std::string userId, email;
        if (!verifyJWT(token, userId, email)) {
            result.error = "Invalid token signature";
            return result;
        }
        
        // Cross-check with cached data
        if (cachedData.contains("userId") && cachedData.contains("email")) {
            if (cachedData["userId"] != userId || cachedData["email"] != email) {
                result.error = "Token data mismatch";
                return result;
            }
        }
        
        result.success = true;
        result.userId = userId;
        result.email = email;
        
    } catch (const std::exception& e) {
        result.error = "Token verification failed: " + std::string(e.what());
    }
    
    return result;
}

bool AuthMiddleware::requireAuth(const httplib::Request& req, httplib::Response& res) {
    AuthResult authResult = authenticate(req);
    
    if (!authResult.success) {
        json errorResponse = createAuthErrorResponse(authResult.error);
        res.status = 401;
        res.set_content(errorResponse.dump(), "application/json");
        return false;
    }
    
    // Store user info in request for use by controllers
    // Note: cpp-httplib doesn't have built-in way to store data in request
    // We'll pass userId through other means in the controllers
    
    return true;
}

std::string AuthMiddleware::extractToken(const std::string& authHeader) {
    const std::string bearerPrefix = "Bearer ";
    
    if (authHeader.length() <= bearerPrefix.length()) {
        return "";
    }
    
    if (authHeader.substr(0, bearerPrefix.length()) != bearerPrefix) {
        return "";
    }
    
    return authHeader.substr(bearerPrefix.length());
}

json AuthMiddleware::createAuthErrorResponse(const std::string& message, int statusCode) {
    return json{
        {"error", message},
        {"status", statusCode},
        {"timestamp", getCurrentTimestamp()}
    };
}

bool AuthMiddleware::verifyJWT(const std::string& token, std::string& userId, std::string& email) {
    try {
        // Decode and verify JWT
        auto decoded = jwt::decode(token);
        
        // Verify signature
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{jwtSecret_})
            .with_issuer("bill-splitter");
        
        verifier.verify(decoded);
        
        // Extract claims
        if (decoded.has_payload_claim("userId")) {
            userId = decoded.get_payload_claim("userId").as_string();
        } else {
            return false;
        }
        
        if (decoded.has_payload_claim("email")) {
            email = decoded.get_payload_claim("email").as_string();
        } else {
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "JWT verification error: " << e.what() << std::endl;
        return false;
    }
}

json AuthMiddleware::parseJWTPayload(const std::string& token) {
    try {
        // Split token into parts
        std::vector<std::string> parts;
        std::stringstream ss(token);
        std::string part;
        
        while (std::getline(ss, part, '.')) {
            parts.push_back(part);
        }
        
        if (parts.size() != 3) {
            return json{};
        }
        
        // Decode payload (second part)
        std::string payload = base64Decode(parts[1]);
        return json::parse(payload);
        
    } catch (const std::exception& e) {
        return json{};
    }
}

std::string AuthMiddleware::base64Decode(const std::string& input) {
    // Simple base64 decode implementation
    // Note: This is a basic implementation, you might want to use a proper library
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string decoded;
    std::vector<int> T(128, -1);
    for (int i = 0; i < 64; i++) T[chars[i]] = i;
    
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return decoded;
}

bool AuthMiddleware::isValidJWTStructure(const std::string& token) {
    // Check if token has the correct JWT structure (3 parts separated by dots)
    std::regex jwtPattern("^[A-Za-z0-9_-]+\\.[A-Za-z0-9_-]+\\.[A-Za-z0-9_-]+$");
    return std::regex_match(token, jwtPattern);
}