#include "thermal/spot_manager/thermal_spot_manager.h"
#include "thermal/spot_manager/spot_persistence.h"
#include "common/logger.h"
#include <algorithm>
#include <cmath>

namespace thermal {

ThermalSpotManager::ThermalSpotManager(std::unique_ptr<TemperatureDataSource> temp_source,
                                     const std::string& persistence_file)
    : temp_source_(std::move(temp_source))
    , persistence_file_path_(persistence_file) {
    
    if (!temp_source_) {
        throw std::invalid_argument("Temperature source cannot be null");
    }
    
    if (!temp_source_->isReady()) {
        LOG_WARN("Temperature source is not ready");
    }
    
    // Load existing spots from persistence
    loadSpots();
    
    LOG_INFO("ThermalSpotManager initialized with " << spots_.size() << " existing spots");
}

ThermalSpotManager::~ThermalSpotManager() {
    // Save spots on destruction
    saveSpots();
}

bool ThermalSpotManager::createSpot(const std::string& spotId, int x, int y) {
    // Validate spot ID
    if (!validateSpotId(spotId)) {
        LOG_ERROR("Invalid spot ID: " << spotId);
        return false;
    }
    
    // Check if spot already exists
    if (spotExists(spotId)) {
        LOG_ERROR("Spot " << spotId << " already exists");
        return false;
    }
    
    // Check maximum spots limit
    if (isMaxSpotsReached()) {
        LOG_ERROR("Maximum spots (" << MAX_SPOTS << ") already reached");
        return false;
    }
    
    // Validate coordinates
    if (!validateCoordinates(x, y)) {
        LOG_ERROR("Invalid coordinates: (" << x << ", " << y << ")");
        return false;
    }
    
    // Create new spot
    auto spot = std::make_unique<MeasurementSpot>();
    spot->id = std::stoi(spotId);  // Convert string ID to int for MeasurementSpot
    spot->name = generateSpotName(spotId);
    spot->enabled = true;
    spot->set_state(SpotState::ACTIVE);
    
    // Configure with temperature source
    configureSpotWithTemperatureSource(*spot, x, y);
    
    // Validate the configured spot
    try {
        if (!spot->validate()) {
            LOG_ERROR("Spot validation failed after configuration");
            return false;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Spot validation error: " << e.what());
        return false;
    }
    
    // Add to spots collection
    spots_[spotId] = std::move(spot);
    
    // Save to persistence
    saveSpots();
    
    LOG_INFO("Created spot " << spotId << " at coordinates (" << x << ", " << y << ")");
    return true;
}

bool ThermalSpotManager::moveSpot(const std::string& spotId, int x, int y) {
    // Check if spot exists
    if (!spotExists(spotId)) {
        LOG_ERROR("Spot " << spotId << " does not exist");
        return false;
    }
    
    // Validate coordinates
    if (!validateCoordinates(x, y)) {
        LOG_ERROR("Invalid coordinates: (" << x << ", " << y << ")");
        return false;
    }
    
    // Update spot coordinates and temperature configuration
    auto& spot = spots_[spotId];
    configureSpotWithTemperatureSource(*spot, x, y);
    
    // Save to persistence
    saveSpots();
    
    LOG_INFO("Moved spot " << spotId << " to coordinates (" << x << ", " << y << ")");
    return true;
}

bool ThermalSpotManager::deleteSpot(const std::string& spotId) {
    // Check if spot exists
    if (!spotExists(spotId)) {
        LOG_ERROR("Spot " << spotId << " does not exist");
        return false;
    }
    
    // Remove from collection
    spots_.erase(spotId);
    
    // Save to persistence
    saveSpots();
    
    LOG_INFO("Deleted spot " << spotId);
    return true;
}

std::vector<MeasurementSpot> ThermalSpotManager::listSpots() const {
    std::vector<MeasurementSpot> result;
    
    for (const auto& [id, spot] : spots_) {
        if (spot) {
            result.push_back(*spot);  // Copy spot for read-only access
        }
    }
    
    return result;
}

float ThermalSpotManager::getSpotTemperature(const std::string& spotId) const {
    if (!spotExists(spotId)) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    
    const auto& spot = spots_.at(spotId);
    
    if (!spot->is_ready() || !temp_source_->isReady()) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    
    // Get temperature from the data source
    return temp_source_->getTemperature(spot->x, spot->y);
}

bool ThermalSpotManager::spotExists(const std::string& spotId) const {
    auto it = spots_.find(spotId);
    return it != spots_.end() && it->second && it->second->is_ready();
}

size_t ThermalSpotManager::getActiveSpotCount() const {
    return spots_.size();
}

bool ThermalSpotManager::isMaxSpotsReached() const {
    return getActiveSpotCount() >= MAX_SPOTS;
}

bool ThermalSpotManager::validateSpotId(const std::string& spotId) {
    return spotId == "1" || spotId == "2" || spotId == "3" || spotId == "4" || spotId == "5";
}

bool ThermalSpotManager::validateCoordinates(int x, int y) const {
    return temp_source_->validateCoordinates(x, y);
}

bool ThermalSpotManager::loadSpots() {
    try {
        SpotPersistence persistence(persistence_file_path_);
        
        std::vector<std::unique_ptr<MeasurementSpot>> loaded_spots;
        if (!persistence.loadSpots(loaded_spots)) {
            LOG_WARN("Failed to load spots from persistence file");
            return false;
        }
        
        // Convert to spot ID mapping
        spots_.clear();
        for (auto& spot : loaded_spots) {
            if (spot && spot->id >= 1 && spot->id <= 5) {
                std::string spotId = std::to_string(spot->id);
                spots_[spotId] = std::move(spot);
            }
        }
        
        LOG_INFO("Loaded " << spots_.size() << " spots from persistence");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception loading spots: " << e.what());
        return false;
    }
}

bool ThermalSpotManager::saveSpots() const {
    try {
        SpotPersistence persistence(persistence_file_path_);
        
        std::vector<std::unique_ptr<MeasurementSpot>> spots_to_save;
        for (const auto& [id, spot] : spots_) {
            if (spot) {
                spots_to_save.push_back(std::make_unique<MeasurementSpot>(*spot));  // Copy for persistence
            }
        }
        
        if (!persistence.saveSpots(spots_to_save)) {
            LOG_WARN("Failed to save spots to persistence file");
            return false;
        }
        
        LOG_DEBUG("Saved " << spots_to_save.size() << " spots to persistence");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception saving spots: " << e.what());
        return false;
    }
}

void ThermalSpotManager::configureSpotWithTemperatureSource(MeasurementSpot& spot, int x, int y) {
    spot.x = x;
    spot.y = y;
    
    if (temp_source_->isReady()) {
        // Get base temperature for this coordinate
        float base_temp = temp_source_->getBaseTemperature(x, y);
        
        // Configure temperature range with ±0.5°C variation
        spot.min_temp = base_temp - 0.5;
        spot.max_temp = base_temp + 0.5;
        spot.noise_factor = 0.1;  // Small noise factor for MeasurementSpot's own variation
    } else {
        LOG_WARN("Temperature source not ready, using default temperature range");
        spot.min_temp = 20.0;
        spot.max_temp = 25.0;
        spot.noise_factor = 0.1;
    }
}

std::string ThermalSpotManager::generateSpotName(const std::string& spotId) const {
    return "thermal_spot_" + spotId;
}

std::chrono::time_point<std::chrono::system_clock> ThermalSpotManager::getCurrentTimestamp() const {
    return std::chrono::system_clock::now();
}

} // namespace thermal