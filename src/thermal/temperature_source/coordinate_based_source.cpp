#include "thermal/temperature_source/coordinate_based_source.h"
#include <cmath>
#include <algorithm>

namespace thermal {

CoordinateBasedTemperatureSource::CoordinateBasedTemperatureSource()
    : gen_(rd_())
    , variation_dist_(-VARIATION_RANGE, VARIATION_RANGE) {
}

float CoordinateBasedTemperatureSource::getTemperature(int x, int y) {
    if (!validateCoordinates(x, y)) {
        return 20.0f; // Default temperature for invalid coordinates
    }
    
    float base_temp = getBaseTemperature(x, y);
    float variation = generateRandomVariation();
    
    return base_temp + variation;
}

bool CoordinateBasedTemperatureSource::isReady() const {
    return true; // Coordinate-based source is always ready
}

std::string CoordinateBasedTemperatureSource::getSourceName() const {
    return "CoordinateBasedTemperatureSource";
}

bool CoordinateBasedTemperatureSource::validateCoordinates(int x, int y) const {
    return x >= 0 && x < IMAGE_WIDTH && y >= 0 && y < IMAGE_HEIGHT;
}

float CoordinateBasedTemperatureSource::getBaseTemperature(int x, int y) const {
    if (!validateCoordinates(x, y)) {
        return MIN_BASE_TEMP; // Return minimum temperature for invalid coordinates
    }
    
    float distance = calculateDistanceFromCenter(x, y);
    
    // Linear interpolation based on distance from center
    // Center (distance 0.0) = MIN_BASE_TEMP (20°C)
    // Corners (distance 1.0) = MAX_BASE_TEMP (50°C)
    float base_temp = MIN_BASE_TEMP + (MAX_BASE_TEMP - MIN_BASE_TEMP) * distance;
    
    return base_temp;
}

float CoordinateBasedTemperatureSource::calculateDistanceFromCenter(int x, int y) const {
    float dx = static_cast<float>(x) - CENTER_X;
    float dy = static_cast<float>(y) - CENTER_Y;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Normalize to 0.0-1.0 range
    // Maximum possible distance is from center to corner
    float max_distance = std::sqrt(CENTER_X * CENTER_X + CENTER_Y * CENTER_Y);
    float normalized_distance = distance / max_distance;
    
    // Clamp to [0.0, 1.0] range
    return std::min(1.0f, std::max(0.0f, normalized_distance));
}

float CoordinateBasedTemperatureSource::generateRandomVariation() const {
    return variation_dist_(gen_);
}

} // namespace thermal