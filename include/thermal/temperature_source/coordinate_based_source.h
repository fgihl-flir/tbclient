#pragma once

#include "thermal/temperature_source/temperature_data_source.h"
#include <random>

namespace thermal {

/**
 * @brief Coordinate-based temperature source for thermal simulation
 * 
 * Implements distance-from-center algorithm with configurable base temperatures
 * and ±0.5°C random variation per reading as specified in clarifications.
 */
class CoordinateBasedTemperatureSource : public TemperatureDataSource {
private:
    mutable std::random_device rd_;
    mutable std::mt19937 gen_;
    mutable std::uniform_real_distribution<float> variation_dist_;
    
    // Image resolution constants
    static constexpr int IMAGE_WIDTH = 320;
    static constexpr int IMAGE_HEIGHT = 240;
    static constexpr float CENTER_X = IMAGE_WIDTH / 2.0f;   // 160.0
    static constexpr float CENTER_Y = IMAGE_HEIGHT / 2.0f;  // 120.0
    
    // Temperature range constants  
    static constexpr float MIN_BASE_TEMP = 20.0f;  // Base temperature at center
    static constexpr float MAX_BASE_TEMP = 50.0f;  // Base temperature at corners
    static constexpr float VARIATION_RANGE = 0.5f; // ±0.5°C random variation
    
public:
    /**
     * @brief Constructor
     */
    CoordinateBasedTemperatureSource();
    
    /**
     * @brief Calculate temperature with coordinate-based algorithm and random variation
     * @param x X coordinate (0-319)
     * @param y Y coordinate (0-239)
     * @return Temperature in Celsius with ±0.5°C variation
     */
    float getTemperature(int x, int y) override;
    
    /**
     * @brief Check if source is ready (always true for coordinate-based)
     * @return true
     */
    bool isReady() const override;
    
    /**
     * @brief Get source name
     * @return "CoordinateBasedTemperatureSource"
     */
    std::string getSourceName() const override;
    
    /**
     * @brief Validate coordinates are within image bounds
     * @param x X coordinate to validate
     * @param y Y coordinate to validate
     * @return true if 0 <= x < 320 and 0 <= y < 240
     */
    bool validateCoordinates(int x, int y) const override;
    
    /**
     * @brief Get base temperature without random variation
     * @param x X coordinate
     * @param y Y coordinate
     * @return Base temperature based on distance from center
     */
    float getBaseTemperature(int x, int y) const override;
    
private:
    /**
     * @brief Calculate distance from center of image
     * @param x X coordinate
     * @param y Y coordinate
     * @return Normalized distance (0.0 at center, 1.0 at corners)
     */
    float calculateDistanceFromCenter(int x, int y) const;
    
    /**
     * @brief Generate random temperature variation
     * @return Random value between -0.5 and +0.5
     */
    float generateRandomVariation() const;
};

} // namespace thermal