#include "utils.h"
#include <cstdlib>
#include <regex>

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

std::string getEnvVar(const std::string& key, const std::string& defaultValue) {
    const char* value = std::getenv(key.c_str());
    return value ? std::string(value) : defaultValue;
}

bool isValidUUID(const std::string& uuid) {
    const std::regex uuidRegex(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
    );
    return std::regex_match(uuid, uuidRegex);
}

json createErrorResponse(const std::string& message, int statusCode) {
    return json{
        {"error", message},
        {"status", statusCode},
        {"timestamp", getCurrentTimestamp()}
    };
}

json createSuccessResponse(const json& data) {
    json response = {
        {"success", true},
        {"timestamp", getCurrentTimestamp()}
    };
    
    if (!data.empty()) {
        response["data"] = data;
    }
    
    return response;
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}