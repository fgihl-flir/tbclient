#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

#include "config/configuration.h"
#include "thermal/measurement_spot.h"
#include "thermal/temperature_reading.h"
#include "common/logger.h"
#include "common/error_handler.h"

namespace thermal {

/**
 * @brief Load configuration from file
 * @param config_file Path to configuration file
 * @return Result with Configuration or error
 */
Result<Configuration> load_configuration(const std::string& config_file) {
    try {
        std::ifstream file(config_file);
        if (!file.is_open()) {
            return Result<Configuration>::error(
                ErrorCode::CONFIG_FILE_NOT_FOUND,
                "Could not open configuration file: " + config_file
            );
        }
        
        nlohmann::json json_data;
        file >> json_data;
        
        Configuration config;
        config.from_json(json_data);
        
        return Result<Configuration>::success(std::move(config));
        
    } catch (const nlohmann::json::exception& e) {
        return Result<Configuration>::error(
            ErrorCode::CONFIG_INVALID_JSON,
            "JSON parsing error: " + std::string(e.what())
        );
    } catch (const std::exception& e) {
        return Result<Configuration>::error(
            ErrorCode::CONFIG_VALIDATION_FAILED,
            "Configuration validation error: " + std::string(e.what())
        );
    }
}

/**
 * @brief Initialize logging from configuration
 * @param config Logging configuration
 */
void initialize_logging(const LoggingConfig& config) {
    LogLevel level = LogLevel::INFO;
    if (config.level == "debug") level = LogLevel::DEBUG;
    else if (config.level == "info") level = LogLevel::INFO;
    else if (config.level == "warn") level = LogLevel::WARN;
    else if (config.level == "error") level = LogLevel::ERROR;
    
    Logger::initialize(level, config.output, config.log_file);
}

/**
 * @brief Create example configuration file
 * @param filename Path to create the configuration file
 */
void create_example_config(const std::string& filename) {
    nlohmann::json config = {
        {"thingsboard", {
            {"host", "localhost"},
            {"port", 1883},
            {"access_token", "YOUR_ACCESS_TOKEN_HERE"},
            {"device_id", "thermal_camera_01"},
            {"use_ssl", false},
            {"keep_alive_seconds", 60},
            {"qos_level", 1}
        }},
        {"telemetry", {
            {"interval_seconds", 15},
            {"batch_transmission", false},
            {"retry_attempts", 3},
            {"retry_delay_ms", 1000},
            {"measurement_spots", {
                {
                    {"id", 1},
                    {"name", "Center Spot"},
                    {"x", 160},
                    {"y", 120},
                    {"min_temp", 20.0},
                    {"max_temp", 80.0},
                    {"noise_factor", 0.1},
                    {"enabled", true}
                }
            }}
        }},
        {"logging", {
            {"level", "info"},
            {"output", "console"},
            {"log_file", "thermal-mqtt.log"}
        }}
    };
    
    std::ofstream file(filename);
    file << config.dump(2);
    file.close();
    
    std::cout << "Created example configuration file: " << filename << std::endl;
}

} // namespace thermal

int main(int argc, char* argv[]) {
    using namespace thermal;
    
    std::string config_file = "config.json";
    
    // Parse command line arguments
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            std::cout << "Thermal Camera MQTT Client (Configuration Test)\n";
            std::cout << "Usage: " << argv[0] << " [config_file]\n";
            std::cout << "       " << argv[0] << " --create-config [filename]\n";
            std::cout << "\nOptions:\n";
            std::cout << "  --create-config  Create example configuration file\n";
            std::cout << "  --help, -h       Show this help message\n";
            return 0;
        } else if (arg == "--create-config") {
            std::string filename = argc > 2 ? argv[2] : "config.example.json";
            create_example_config(filename);
            return 0;
        } else {
            config_file = arg;
        }
    }
    
    std::cout << "Thermal Camera MQTT Client - Configuration Test\n";
    std::cout << "Loading configuration from: " << config_file << std::endl;
    
    // Load configuration
    auto config_result = load_configuration(config_file);
    if (!config_result) {
        std::cerr << "Configuration error: " << config_result.error_message() << std::endl;
        std::cerr << "Try running: " << argv[0] << " --create-config" << std::endl;
        return 1;
    }
    
    Configuration config = config_result.value();
    
    // Initialize logging
    initialize_logging(config.logging_config);
    LOG_INFO("Configuration test starting...");
    
    try {
        // Test measurement spots
        if (config.telemetry_config.measurement_spots.empty()) {
            LOG_ERROR("No measurement spots configured");
            return 1;
        }
        
        for (auto& spot : config.telemetry_config.measurement_spots) {
            spot.set_state(SpotState::ACTIVE);
            
            LOG_INFO("Measurement spot: " << spot.name << " (ID: " << spot.id << ")");
            LOG_INFO("  Position: (" << spot.x << ", " << spot.y << ")");
            LOG_INFO("  Temperature range: " << spot.min_temp << "°C - " << spot.max_temp << "°C");
            
            // Test temperature generation
            if (spot.is_ready()) {
                double temp = spot.generate_temperature();
                LOG_INFO("  Generated temperature: " << temp << "°C");
                
                // Test temperature reading creation
                TemperatureReading reading(spot.id, temp);
                if (reading.validate()) {
                    LOG_INFO("  Temperature reading validation: PASS");
                } else {
                    LOG_ERROR("  Temperature reading validation: FAIL");
                }
            }
        }
        
        LOG_INFO("Configuration test completed successfully");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Test error: " << e.what());
        return 1;
    }
    
    LOG_INFO("Test completed - core functionality working");
    return 0;
}