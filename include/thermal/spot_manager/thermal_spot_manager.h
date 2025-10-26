#pragma once

#include "thermal/measurement_spot.h"
#include "thermal/temperature_source/temperature_data_source.h"
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <chrono>

namespace thermal {

/**
 * @brief Central manager for thermal measurement spots with RPC control
 * 
 * Coordinates spot lifecycle (create, move, delete), integrates with temperature
 * data sources, and manages spot persistence. Extends existing MeasurementSpot
 * infrastructure with RPC capabilities.
 */
class ThermalSpotManager {
private:
    // Maximum spots per FR-014 requirement
    static constexpr size_t MAX_SPOTS = 5;
    
    // Active spots indexed by spotId ("1" to "5")
    std::map<std::string, std::unique_ptr<MeasurementSpot>> spots_;
    
    // Temperature data source for coordinate-based calculation
    std::unique_ptr<TemperatureDataSource> temp_source_;
    
    // Persistence manager for spot configuration
    std::string persistence_file_path_;
    
public:
    /**
     * @brief Constructor with just config path
     * @param persistence_file Path to JSON persistence file
     */
    explicit ThermalSpotManager(const std::string& persistence_file = "thermal_spots.json");
    
    /**
     * @brief Constructor with temperature source
     * @param temp_source Temperature data source for spot calculation
     * @param persistence_file Path to JSON persistence file (default: "thermal_spots.json")
     */
    explicit ThermalSpotManager(std::unique_ptr<TemperatureDataSource> temp_source,
                               const std::string& persistence_file = "thermal_spots.json");
    
    /**
     * @brief Destructor - saves spots to persistence file
     */
    ~ThermalSpotManager();
    
    /**
     * @brief Set temperature data source
     * @param temp_source Temperature data source for coordinate-based calculation
     */
    void setTemperatureSource(std::unique_ptr<TemperatureDataSource> temp_source);
    
    /**
     * @brief Create new measurement spot at specified coordinates
     * @param spotId Spot identifier ("1" to "5")
     * @param x X coordinate (0-319)
     * @param y Y coordinate (0-239)
     * @return true if spot created successfully
     */
    bool createSpot(const std::string& spotId, int x, int y);
    
    /**
     * @brief Move existing spot to new coordinates
     * @param spotId Spot identifier ("1" to "5")
     * @param x New X coordinate (0-319)
     * @param y New Y coordinate (0-239)
     * @return true if spot moved successfully
     */
    bool moveSpot(const std::string& spotId, int x, int y);
    
    /**
     * @brief Delete measurement spot
     * @param spotId Spot identifier ("1" to "5")
     * @return true if spot deleted successfully
     */
    bool deleteSpot(const std::string& spotId);
    
    /**
     * @brief Get list of all active spots
     * @return Vector of spot copies for reading
     */
    std::vector<MeasurementSpot> listSpots() const;
    
    /**
     * @brief Get current temperature reading for a spot (simple version)
     * @param spotId Spot identifier ("1" to "5")
     * @return Temperature in Celsius, or NaN if spot doesn't exist
     */
    float getSpotTemperature(const std::string& spotId) const;
    
    /**
     * @brief Check if spot exists and is active
     * @param spotId Spot identifier to check
     * @return true if spot exists and is ready for measurement
     */
    bool spotExists(const std::string& spotId) const;
    
    /**
     * @brief Get number of currently active spots
     * @return Count of active spots (0-5)
     */
    size_t getActiveSpotCount() const;
    
    /**
     * @brief Check if maximum spot limit is reached
     * @return true if 5 spots are already active
     */
    bool isMaxSpotsReached() const;
    
    /**
     * @brief Validate spot ID format
     * @param spotId Spot ID to validate
     * @return true if spotId is "1", "2", "3", "4", or "5"
     */
    static bool validateSpotId(const std::string& spotId);
    
    /**
     * @brief Validate coordinates for thermal image
     * @param x X coordinate
     * @param y Y coordinate
     * @return true if coordinates are within 320x240 bounds
     */
    bool validateCoordinates(int x, int y) const;
    
    /**
     * @brief Load spots from persistence file
     * @return true if loading was successful (or file doesn't exist)
     */
    bool loadSpots();
    
    /**
     * @brief Save spots to persistence file
     * @return true if saving was successful
     */
    bool saveSpots() const;
    
private:
    /**
     * @brief Configure MeasurementSpot with temperature source data
     * @param spot MeasurementSpot to configure
     * @param x X coordinate
     * @param y Y coordinate
     */
    void configureSpotWithTemperatureSource(MeasurementSpot& spot, int x, int y);
    
    /**
     * @brief Generate spot name from ID
     * @param spotId Spot identifier
     * @return Generated spot name
     */
    std::string generateSpotName(const std::string& spotId) const;
    
    /**
     * @brief Get current timestamp
     * @return Current system time
     */
    std::chrono::time_point<std::chrono::system_clock> getCurrentTimestamp() const;
};

} // namespace thermal