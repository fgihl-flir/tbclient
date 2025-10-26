#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <set>

namespace thermal {

/**
 * @brief Measurement spot states
 */
enum class SpotState {
    INACTIVE,    // Spot is disabled
    ACTIVE,      // Spot is enabled and ready
    READING,     // Currently taking a measurement
    ERROR        // Spot has an error condition
};

/**
 * @brief Single thermal measurement point with configuration and state
 */
struct MeasurementSpot {
    // Configuration
    int id = 0;
    std::string name;
    int x = 0;                  // X coordinate in thermal image (pixels)
    int y = 0;                  // Y coordinate in thermal image (pixels)
    double min_temp = 20.0;     // Minimum expected temperature (°C)
    double max_temp = 100.0;    // Maximum expected temperature (°C)
    double noise_factor = 0.1;  // Temperature variation noise factor (0.0-1.0)
    bool enabled = true;        // Whether this spot is actively monitored
    
    // RPC-specific metadata (optional)
    std::string created_at;     // ISO 8601 timestamp when spot was created via RPC
    std::string last_reading_at; // ISO 8601 timestamp of last temperature reading
    
    // Runtime state
    SpotState state = SpotState::INACTIVE;
    
    /**
     * @brief Validate the measurement spot configuration
     * @return true if configuration is valid
     * @throws std::invalid_argument if validation fails
     */
    bool validate() const;
    
    /**
     * @brief Load spot configuration from JSON
     * @param json_data The JSON object to parse
     * @throws std::invalid_argument if JSON is invalid
     */
    void from_json(const nlohmann::json& json_data);
    
    /**
     * @brief Convert spot configuration to JSON
     * @return JSON representation of the spot
     */
    nlohmann::json to_json() const;
    
    /**
     * @brief Generate a simulated temperature reading for this spot
     * @return Temperature value in Celsius
     */
    double generate_temperature() const;
    
    /**
     * @brief Check if temperature is within expected range for this spot
     * @param temperature Temperature to check
     * @return true if temperature is within expected range
     */
    bool is_temperature_expected(double temperature) const;
    
    /**
     * @brief Update the spot's runtime state
     * @param new_state New state to set
     */
    void set_state(SpotState new_state);
    
    /**
     * @brief Get the current state
     * @return Current spot state
     */
    SpotState get_state() const;
    
    /**
     * @brief Check if spot is ready for measurement
     * @return true if enabled and in ACTIVE state
     */
    bool is_ready() const;
};

} // namespace thermal