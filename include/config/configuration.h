#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>

namespace thermal {

// Forward declaration for MeasurementSpot
struct MeasurementSpot;

/**
 * @brief ThingsBoard-specific connection and authentication parameters
 */
struct ThingsBoardConfig {
    std::string host;
    int port = 1883;
    std::string access_token;
    std::string device_id;
    bool use_ssl = false;
    int keep_alive_seconds = 60;
    int qos_level = 1;

    bool validate() const;
    void from_json(const nlohmann::json& json_data);
    nlohmann::json to_json() const;
};

/**
 * @brief Telemetry transmission parameters and measurement spot configurations
 */
struct TelemetryConfig {
    int interval_seconds = 15;
    std::vector<MeasurementSpot> measurement_spots;
    bool batch_transmission = false;  // Send individual messages per clarification
    int retry_attempts = 3;
    int retry_delay_ms = 1000;

    bool validate() const;
    void from_json(const nlohmann::json& json_data);
    nlohmann::json to_json() const;
};

/**
 * @brief Logging configuration
 */
struct LoggingConfig {
    std::string level = "info";  // debug, info, warn, error
    std::string output = "console";  // console, file, both
    std::string log_file = "thermal-mqtt.log";

    bool validate() const;
    void from_json(const nlohmann::json& json_data);
    nlohmann::json to_json() const;
};

/**
 * @brief Complete application configuration loaded from JSON files
 */
struct Configuration {
    ThingsBoardConfig thingsboard_config;
    TelemetryConfig telemetry_config;
    LoggingConfig logging_config;

    /**
     * @brief Load configuration from JSON file
     * @param filename Path to the JSON configuration file
     * @throws std::invalid_argument if file cannot be read or JSON is invalid
     */
    void load_from_file(const std::string& filename);

    /**
     * @brief Validate the complete configuration
     * @return true if configuration is valid
     * @throws std::invalid_argument if validation fails
     */
    bool validate() const;

    /**
     * @brief Load configuration from JSON
     * @param json_data The JSON object to parse
     * @throws std::invalid_argument if JSON is invalid
     */
    void from_json(const nlohmann::json& json_data);

    /**
     * @brief Convert configuration to JSON
     * @return JSON representation of configuration
     */
    nlohmann::json to_json() const;
};

/**
 * @brief Configuration-related errors
 */
enum class ConfigurationErrorType {
    MISSING_FILE,
    INVALID_JSON,
    VALIDATION_FAILED
};

/**
 * @brief Configuration error information
 */
struct ConfigurationError {
    ConfigurationErrorType error_type;
    std::string field_path;
    std::string error_message;
    
    ConfigurationError(ConfigurationErrorType type, const std::string& path, const std::string& message)
        : error_type(type), field_path(path), error_message(message) {}
};

} // namespace thermal