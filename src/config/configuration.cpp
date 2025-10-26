#include "config/configuration.h"
#include "thermal/measurement_spot.h"
#include <stdexcept>
#include <regex>
#include <set>
#include <fstream>

namespace thermal {

void Configuration::load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::invalid_argument("Could not open configuration file: " + filename);
    }
    
    nlohmann::json json_data;
    file >> json_data;
    
    from_json(json_data);
}

// Configuration validation and JSON handling
bool Configuration::validate() const {
    return thingsboard_config.validate() && 
           telemetry_config.validate() && 
           logging_config.validate();
}

void Configuration::from_json(const nlohmann::json& json_data) {
    try {
        if (json_data.contains("thingsboard")) {
            thingsboard_config.from_json(json_data["thingsboard"]);
        } else {
            throw std::invalid_argument("Missing 'thingsboard' configuration section");
        }

        if (json_data.contains("telemetry")) {
            telemetry_config.from_json(json_data["telemetry"]);
        } else {
            throw std::invalid_argument("Missing 'telemetry' configuration section");
        }

        if (json_data.contains("logging")) {
            logging_config.from_json(json_data["logging"]);
        }
        // logging_config has defaults, so it's optional

        if (!validate()) {
            throw std::invalid_argument("Configuration validation failed");
        }
    } catch (const nlohmann::json::exception& e) {
        throw std::invalid_argument("JSON parsing error: " + std::string(e.what()));
    }
}

nlohmann::json Configuration::to_json() const {
    nlohmann::json json_data;
    json_data["thingsboard"] = thingsboard_config.to_json();
    json_data["telemetry"] = telemetry_config.to_json();
    json_data["logging"] = logging_config.to_json();
    return json_data;
}

// ThingsBoardConfig implementation
bool ThingsBoardConfig::validate() const {
    if (host.empty()) {
        throw std::invalid_argument("ThingsBoard host cannot be empty");
    }
    
    if (port < 1 || port > 65535) {
        throw std::invalid_argument("Port must be between 1 and 65535");
    }
    
    if (access_token.empty()) {
        throw std::invalid_argument("Access token cannot be empty");
    }
    
    if (device_id.empty()) {
        throw std::invalid_argument("Device ID cannot be empty");
    }
    
    // Device ID validation: alphanumeric and hyphens only
    std::regex device_id_pattern("^[a-zA-Z0-9_-]+$");
    if (!std::regex_match(device_id, device_id_pattern)) {
        throw std::invalid_argument("Device ID must contain only alphanumeric characters, underscores, and hyphens");
    }
    
    if (keep_alive_seconds < 10 || keep_alive_seconds > 300) {
        throw std::invalid_argument("Keep alive must be between 10 and 300 seconds");
    }
    
    if (qos_level < 0 || qos_level > 2) {
        throw std::invalid_argument("QoS level must be 0, 1, or 2");
    }
    
    return true;
}

void ThingsBoardConfig::from_json(const nlohmann::json& json_data) {
    if (json_data.contains("host")) {
        host = json_data["host"].get<std::string>();
    }
    if (json_data.contains("port")) {
        port = json_data["port"].get<int>();
    }
    if (json_data.contains("access_token")) {
        access_token = json_data["access_token"].get<std::string>();
    }
    if (json_data.contains("device_id")) {
        device_id = json_data["device_id"].get<std::string>();
    }
    if (json_data.contains("use_ssl")) {
        use_ssl = json_data["use_ssl"].get<bool>();
    }
    if (json_data.contains("keep_alive_seconds")) {
        keep_alive_seconds = json_data["keep_alive_seconds"].get<int>();
    }
    if (json_data.contains("qos_level")) {
        qos_level = json_data["qos_level"].get<int>();
    }
}

nlohmann::json ThingsBoardConfig::to_json() const {
    return nlohmann::json{
        {"host", host},
        {"port", port},
        {"access_token", access_token},
        {"device_id", device_id},
        {"use_ssl", use_ssl},
        {"keep_alive_seconds", keep_alive_seconds},
        {"qos_level", qos_level}
    };
}

// TelemetryConfig implementation
bool TelemetryConfig::validate() const {
    if (interval_seconds < 1 || interval_seconds > 3600) {
        throw std::invalid_argument("Telemetry interval must be between 1 and 3600 seconds");
    }
    
    // Allow empty measurement_spots array - spots can be created dynamically via RPC
    if (measurement_spots.size() > 5) {
        throw std::invalid_argument("Maximum 5 measurement spots allowed");
    }
    
    if (retry_attempts < 0 || retry_attempts > 10) {
        throw std::invalid_argument("Retry attempts must be between 0 and 10");
    }
    
    if (retry_delay_ms < 100 || retry_delay_ms > 10000) {
        throw std::invalid_argument("Retry delay must be between 100 and 10000 milliseconds");
    }
    
    // Validate each measurement spot
    for (const auto& spot : measurement_spots) {
        if (!spot.validate()) {
            return false;
        }
    }
    
    // Check for unique spot IDs
    std::set<int> spot_ids;
    for (const auto& spot : measurement_spots) {
        if (spot_ids.count(spot.id) > 0) {
            throw std::invalid_argument("Duplicate measurement spot ID: " + std::to_string(spot.id));
        }
        spot_ids.insert(spot.id);
    }
    
    return true;
}

void TelemetryConfig::from_json(const nlohmann::json& json_data) {
    if (json_data.contains("interval_seconds")) {
        interval_seconds = json_data["interval_seconds"].get<int>();
    }
    if (json_data.contains("batch_transmission")) {
        batch_transmission = json_data["batch_transmission"].get<bool>();
    }
    if (json_data.contains("retry_attempts")) {
        retry_attempts = json_data["retry_attempts"].get<int>();
    }
    if (json_data.contains("retry_delay_ms")) {
        retry_delay_ms = json_data["retry_delay_ms"].get<int>();
    }
    if (json_data.contains("measurement_spots")) {
        measurement_spots.clear();
        for (const auto& spot_json : json_data["measurement_spots"]) {
            MeasurementSpot spot;
            spot.from_json(spot_json);
            measurement_spots.push_back(spot);
        }
    }
}

nlohmann::json TelemetryConfig::to_json() const {
    nlohmann::json spots_json = nlohmann::json::array();
    for (const auto& spot : measurement_spots) {
        spots_json.push_back(spot.to_json());
    }
    
    return nlohmann::json{
        {"interval_seconds", interval_seconds},
        {"batch_transmission", batch_transmission},
        {"retry_attempts", retry_attempts},
        {"retry_delay_ms", retry_delay_ms},
        {"measurement_spots", spots_json}
    };
}

// LoggingConfig implementation
bool LoggingConfig::validate() const {
    const std::set<std::string> valid_levels = {"debug", "info", "warn", "error"};
    if (valid_levels.count(level) == 0) {
        throw std::invalid_argument("Invalid log level: " + level);
    }
    
    const std::set<std::string> valid_outputs = {"console", "file", "both"};
    if (valid_outputs.count(output) == 0) {
        throw std::invalid_argument("Invalid log output: " + output);
    }
    
    if (output == "file" || output == "both") {
        if (log_file.empty()) {
            throw std::invalid_argument("Log file cannot be empty when file output is enabled");
        }
    }
    
    return true;
}

void LoggingConfig::from_json(const nlohmann::json& json_data) {
    if (json_data.contains("level")) {
        level = json_data["level"].get<std::string>();
    }
    if (json_data.contains("output")) {
        output = json_data["output"].get<std::string>();
    }
    if (json_data.contains("log_file")) {
        log_file = json_data["log_file"].get<std::string>();
    }
}

nlohmann::json LoggingConfig::to_json() const {
    return nlohmann::json{
        {"level", level},
        {"output", output},
        {"log_file", log_file}
    };
}

} // namespace thermal