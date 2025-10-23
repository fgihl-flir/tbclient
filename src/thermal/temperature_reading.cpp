#include "thermal/temperature_reading.h"
#include <stdexcept>
#include <iomanip>
#include <sstream>

namespace thermal {

bool TemperatureReading::validate() const {
    if (spot_id <= 0) {
        throw std::invalid_argument("Spot ID must be positive");
    }
    
    if (!is_temperature_valid(temperature)) {
        throw std::invalid_argument("Temperature must be between -100°C and 500°C");
    }
    
    // Check timestamp is not in the future
    auto now = std::chrono::system_clock::now();
    if (timestamp > now) {
        throw std::invalid_argument("Timestamp cannot be in the future");
    }
    
    // If quality is ERROR, error_code must be present
    if (quality == ReadingQuality::ERROR && !error_code.has_value()) {
        throw std::invalid_argument("Error code required when quality is ERROR");
    }
    
    return true;
}

void TemperatureReading::from_json(const nlohmann::json& json_data) {
    if (json_data.contains("spot_id")) {
        spot_id = json_data["spot_id"].get<int>();
    }
    if (json_data.contains("temperature")) {
        temperature = json_data["temperature"].get<double>();
    }
    if (json_data.contains("quality")) {
        std::string quality_str = json_data["quality"].get<std::string>();
        quality = string_to_quality(quality_str);
    }
    if (json_data.contains("error_code")) {
        error_code = json_data["error_code"].get<int>();
    }
    if (json_data.contains("timestamp_ms")) {
        auto timestamp_ms = json_data["timestamp_ms"].get<long long>();
        timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestamp_ms));
    }
}

nlohmann::json TemperatureReading::to_json() const {
    nlohmann::json json_data;
    json_data["spot_id"] = spot_id;
    json_data["temperature"] = temperature;
    json_data["quality"] = quality_to_string(quality);
    
    if (error_code.has_value()) {
        json_data["error_code"] = error_code.value();
    }
    
    // Convert timestamp to milliseconds since epoch
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();
    json_data["timestamp_ms"] = timestamp_ms;
    
    return json_data;
}

bool TemperatureReading::is_temperature_valid(double temperature) {
    return temperature >= -100.0 && temperature <= 500.0;
}

std::string TemperatureReading::quality_to_string(ReadingQuality quality) {
    switch (quality) {
        case ReadingQuality::GOOD:    return "GOOD";
        case ReadingQuality::POOR:    return "POOR";
        case ReadingQuality::INVALID: return "INVALID";
        case ReadingQuality::ERROR:   return "ERROR";
        default: return "UNKNOWN";
    }
}

ReadingQuality TemperatureReading::string_to_quality(const std::string& quality_str) {
    if (quality_str == "GOOD")    return ReadingQuality::GOOD;
    if (quality_str == "POOR")    return ReadingQuality::POOR;
    if (quality_str == "INVALID") return ReadingQuality::INVALID;
    if (quality_str == "ERROR")   return ReadingQuality::ERROR;
    
    throw std::invalid_argument("Unknown quality string: " + quality_str);
}

} // namespace thermal