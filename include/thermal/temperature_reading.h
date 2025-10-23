#pragma once

#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>

namespace thermal {

/**
 * @brief Temperature reading quality indicators
 */
enum class ReadingQuality {
    GOOD,     // Measurement within expected range and confidence
    POOR,     // Measurement outside normal range but possibly valid
    INVALID,  // Measurement clearly invalid (sensor error, out of range)
    ERROR     // Measurement failed due to system error
};

/**
 * @brief Single temperature measurement from a specific spot at a point in time
 */
struct TemperatureReading {
    int spot_id = 0;                                    // ID of the measurement spot
    double temperature = 0.0;                           // Temperature value in Celsius
    std::chrono::time_point<std::chrono::system_clock> timestamp;  // When measurement was taken
    ReadingQuality quality = ReadingQuality::GOOD;     // Measurement quality indicator
    std::optional<int> error_code;                      // Error code if measurement failed
    
    /**
     * @brief Default constructor with current timestamp
     */
    TemperatureReading() : timestamp(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Constructor with parameters
     * @param spot_id ID of measurement spot
     * @param temperature Temperature value in Celsius
     * @param quality Reading quality (default: GOOD)
     */
    TemperatureReading(int spot_id, double temperature, ReadingQuality quality = ReadingQuality::GOOD)
        : spot_id(spot_id), temperature(temperature), timestamp(std::chrono::system_clock::now()), quality(quality) {}
    
    /**
     * @brief Validate the temperature reading
     * @return true if reading is valid
     * @throws std::invalid_argument if validation fails
     */
    bool validate() const;
    
    /**
     * @brief Load reading from JSON
     * @param json_data The JSON object to parse
     * @throws std::invalid_argument if JSON is invalid
     */
    void from_json(const nlohmann::json& json_data);
    
    /**
     * @brief Convert reading to JSON
     * @return JSON representation of the reading
     */
    nlohmann::json to_json() const;
    
    /**
     * @brief Check if temperature is within global validation range
     * @param temperature Temperature to check
     * @return true if within -100°C to 500°C range
     */
    static bool is_temperature_valid(double temperature);
    
    /**
     * @brief Convert ReadingQuality enum to string
     * @param quality Quality enum value
     * @return String representation
     */
    static std::string quality_to_string(ReadingQuality quality);
    
    /**
     * @brief Convert string to ReadingQuality enum
     * @param quality_str String representation
     * @return Quality enum value
     */
    static ReadingQuality string_to_quality(const std::string& quality_str);
};

} // namespace thermal