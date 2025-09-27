#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string getCurrentTimestamp();
std::string getEnvVar(const std::string& key, const std::string& defaultValue = "");
bool isValidUUID(const std::string& uuid);
json createErrorResponse(const std::string& message, int statusCode = 400);
json createSuccessResponse(const json& data = json::object());
std::string trim(const std::string& str);

#endif