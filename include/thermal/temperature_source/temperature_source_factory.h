#pragma once

#include "thermal/temperature_source/temperature_data_source.h"
#include <memory>
#include <string>

namespace thermal {

/**
 * @brief Factory for creating temperature data source instances
 * 
 * Enables modular temperature source selection for future extensibility
 * with remote data sources while maintaining current coordinate-based simulation.
 */
class TemperatureSourceFactory {
public:
    /**
     * @brief Temperature source types
     */
    enum class SourceType {
        COORDINATE_BASED,  // Current coordinate-based simulation
        REMOTE_HTTP,       // Future: HTTP API integration
        REMOTE_MQTT        // Future: MQTT data stream integration
    };
    
    /**
     * @brief Create temperature data source instance
     * @param type Type of temperature source to create
     * @return Unique pointer to temperature data source
     */
    static std::unique_ptr<TemperatureDataSource> createSource(SourceType type);
    
    /**
     * @brief Create temperature data source from string
     * @param type_str String representation of source type
     * @return Unique pointer to temperature data source
     */
    static std::unique_ptr<TemperatureDataSource> createSource(const std::string& type_str);
    
    /**
     * @brief Get default temperature source (coordinate-based)
     * @return Unique pointer to default temperature data source
     */
    static std::unique_ptr<TemperatureDataSource> createDefault();
    
    /**
     * @brief Convert source type to string
     * @param type Source type enum
     * @return String representation
     */
    static std::string sourceTypeToString(SourceType type);
    
    /**
     * @brief Parse source type from string
     * @param type_str String representation
     * @return Source type enum
     */
    static SourceType parseSourceType(const std::string& type_str);
};

} // namespace thermal