#include "thermal/temperature_source/temperature_source_factory.h"
#include "thermal/temperature_source/coordinate_based_source.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace thermal {

std::unique_ptr<TemperatureDataSource> TemperatureSourceFactory::createSource(SourceType type) {
    switch (type) {
        case SourceType::COORDINATE_BASED:
            return std::make_unique<CoordinateBasedTemperatureSource>();
        case SourceType::REMOTE_HTTP:
            // Future implementation: HTTP API integration
            throw std::runtime_error("HTTP temperature source not yet implemented");
        case SourceType::REMOTE_MQTT:
            // Future implementation: MQTT data stream integration
            throw std::runtime_error("MQTT temperature source not yet implemented");
        default:
            throw std::invalid_argument("Unknown temperature source type");
    }
}

std::unique_ptr<TemperatureDataSource> TemperatureSourceFactory::createSource(const std::string& type_str) {
    SourceType type = parseSourceType(type_str);
    return createSource(type);
}

std::unique_ptr<TemperatureDataSource> TemperatureSourceFactory::createDefault() {
    return createSource(SourceType::COORDINATE_BASED);
}

std::string TemperatureSourceFactory::sourceTypeToString(SourceType type) {
    switch (type) {
        case SourceType::COORDINATE_BASED:
            return "coordinate_based";
        case SourceType::REMOTE_HTTP:
            return "remote_http";
        case SourceType::REMOTE_MQTT:
            return "remote_mqtt";
        default:
            return "unknown";
    }
}

TemperatureSourceFactory::SourceType TemperatureSourceFactory::parseSourceType(const std::string& type_str) {
    // Convert to lowercase for case-insensitive comparison
    std::string lower_str = type_str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    if (lower_str == "coordinate_based") {
        return SourceType::COORDINATE_BASED;
    } else if (lower_str == "remote_http") {
        return SourceType::REMOTE_HTTP;
    } else if (lower_str == "remote_mqtt") {
        return SourceType::REMOTE_MQTT;
    } else {
        throw std::invalid_argument("Unknown temperature source type: " + type_str);
    }
}

} // namespace thermal