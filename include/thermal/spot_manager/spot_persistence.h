#pragma once

#include "thermal/measurement_spot.h"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace thermal {

/**
 * @brief JSON-based persistence manager for thermal measurement spots
 * 
 * Handles loading and saving spot configurations to JSON files with
 * graceful error handling and corruption recovery as specified in research.md.
 */
class SpotPersistence {
private:
    std::string file_path_;
    
    // Schema version for future migration support
    static constexpr const char* SCHEMA_VERSION = "1.0";
    
public:
    /**
     * @brief Constructor
     * @param file_path Path to JSON persistence file
     */
    explicit SpotPersistence(const std::string& file_path);
    
    /**
     * @brief Load spots from JSON file
     * @param spots Vector to populate with loaded spots
     * @return true if loading was successful (or file doesn't exist)
     */
    bool loadSpots(std::vector<std::unique_ptr<MeasurementSpot>>& spots);
    
    /**
     * @brief Save spots to JSON file
     * @param spots Vector of spots to save
     * @return true if saving was successful
     */
    bool saveSpots(const std::vector<std::unique_ptr<MeasurementSpot>>& spots);
    
    /**
     * @brief Check if persistence file exists
     * @return true if file exists and is readable
     */
    bool fileExists() const;
    
    /**
     * @brief Create backup of current file
     * @return true if backup was created successfully
     */
    bool createBackup() const;
    
    /**
     * @brief Validate JSON schema version
     * @param json JSON object to validate
     * @return true if schema is compatible
     */
    static bool validateSchema(const nlohmann::json& json);
    
private:
    /**
     * @brief Load single spot from JSON object
     * @param spot_json JSON object representing a spot
     * @return Unique pointer to loaded spot, or nullptr if invalid
     */
    std::unique_ptr<MeasurementSpot> loadSpotFromJson(const nlohmann::json& spot_json);
    
    /**
     * @brief Convert spot to JSON object with RPC metadata
     * @param spot Spot to convert
     * @return JSON representation
     */
    nlohmann::json spotToJson(const MeasurementSpot& spot);
    
    /**
     * @brief Generate backup file path
     * @return Path for backup file with timestamp
     */
    std::string generateBackupPath() const;
    
    /**
     * @brief Get current timestamp string
     * @return ISO 8601 timestamp
     */
    std::string getCurrentTimestamp() const;
};

} // namespace thermal