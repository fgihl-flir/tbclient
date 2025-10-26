#pragma once

#include <string>

namespace thermal {

/**
 * @brief Abstract interface for temperature data sources
 * 
 * Provides modular temperature calculation strategies for thermal simulation.
 * Enables future integration with remote temperature sources while maintaining
 * a simple synchronous interface for the current proof-of-concept.
 */
class TemperatureDataSource {
public:
    virtual ~TemperatureDataSource() = default;
    
    /**
     * @brief Calculate temperature for given coordinates
     * @param x X coordinate (0-319 for 320px width)
     * @param y Y coordinate (0-239 for 240px height)
     * @return Temperature in Celsius (-40°C to +150°C range)
     */
    virtual float getTemperature(int x, int y) = 0;
    
    /**
     * @brief Check if the data source is ready to provide temperatures
     * @return true if source can provide temperature data
     */
    virtual bool isReady() const = 0;
    
    /**
     * @brief Get human-readable name of the data source
     * @return Data source identifier
     */
    virtual std::string getSourceName() const = 0;
    
    /**
     * @brief Validate coordinate bounds for this data source
     * @param x X coordinate to validate
     * @param y Y coordinate to validate  
     * @return true if coordinates are within valid bounds
     */
    virtual bool validateCoordinates(int x, int y) const = 0;
    
    /**
     * @brief Get base temperature without random variation
     * @param x X coordinate
     * @param y Y coordinate
     * @return Base temperature for coordinate-based calculation
     */
    virtual float getBaseTemperature(int x, int y) const = 0;
};

} // namespace thermal