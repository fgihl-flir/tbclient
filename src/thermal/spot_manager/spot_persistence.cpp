#include "thermal/spot_manager/spot_persistence.h"
#include "common/logger.h"
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <chrono>

namespace thermal {

SpotPersistence::SpotPersistence(const std::string& file_path)
    : file_path_(file_path) {
}

bool SpotPersistence::loadSpots(std::vector<std::unique_ptr<MeasurementSpot>>& spots) {
    spots.clear();
    
    if (!fileExists()) {
        LOG_INFO("Spot persistence file not found: " << file_path_ << " (starting with empty spots)");
        return true; // Not an error - start with empty spots
    }
    
    try {
        std::ifstream file(file_path_);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open spot persistence file: " << file_path_);
            return false;
        }
        
        nlohmann::json json_data;
        file >> json_data;
        file.close();
        
        if (!validateSchema(json_data)) {
            LOG_WARN("Invalid schema in spot persistence file, starting with empty spots");
            return true; // Graceful degradation
        }
        
        if (!json_data.contains("thermal_spots") || !json_data["thermal_spots"].is_array()) {
            LOG_WARN("No thermal_spots array found, starting with empty spots");
            return true;
        }
        
        const auto& spots_array = json_data["thermal_spots"];
        for (const auto& spot_json : spots_array) {
            auto spot = loadSpotFromJson(spot_json);
            if (spot) {
                spots.push_back(std::move(spot));
                LOG_DEBUG("Loaded spot ID " << spots.back()->id << " from persistence");
            }
        }
        
        LOG_INFO("Loaded " << spots.size() << " spots from " << file_path_);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception loading spots from " << file_path_ << ": " << e.what());
        LOG_INFO("Starting with empty spots due to file corruption");
        spots.clear();
        return true; // Graceful degradation
    }
}

bool SpotPersistence::saveSpots(const std::vector<std::unique_ptr<MeasurementSpot>>& spots) {
    try {
        // Create backup before saving
        if (fileExists()) {
            createBackup();
        }
        
        nlohmann::json json_data;
        json_data["version"] = SCHEMA_VERSION;
        json_data["lastUpdated"] = getCurrentTimestamp();
        json_data["totalActiveSpots"] = spots.size();
        
        nlohmann::json spots_array = nlohmann::json::array();
        for (const auto& spot : spots) {
            if (spot) {
                spots_array.push_back(spotToJson(*spot));
            }
        }
        json_data["thermal_spots"] = spots_array;
        
        std::ofstream file(file_path_);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open spot persistence file for writing: " << file_path_);
            return false;
        }
        
        file << std::setw(2) << json_data << std::endl;
        file.close();
        
        LOG_DEBUG("Saved " << spots.size() << " spots to " << file_path_);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception saving spots to " << file_path_ << ": " << e.what());
        return false;
    }
}

bool SpotPersistence::fileExists() const {
    return std::filesystem::exists(file_path_) && std::filesystem::is_regular_file(file_path_);
}

bool SpotPersistence::createBackup() const {
    if (!fileExists()) {
        return true; // No file to backup
    }
    
    try {
        std::string backup_path = generateBackupPath();
        std::filesystem::copy_file(file_path_, backup_path);
        LOG_DEBUG("Created backup: " << backup_path);
        return true;
    } catch (const std::exception& e) {
        LOG_WARN("Failed to create backup of " << file_path_ << ": " << e.what());
        return false; // Not a fatal error
    }
}

bool SpotPersistence::validateSchema(const nlohmann::json& json) {
    if (!json.is_object()) {
        return false;
    }
    
    // Check for required fields
    if (!json.contains("version") || !json["version"].is_string()) {
        return false;
    }
    
    // For now, only support version 1.0
    std::string version = json["version"].get<std::string>();
    if (version != SCHEMA_VERSION) {
        LOG_WARN("Unsupported schema version: " << version << " (expected: " << SCHEMA_VERSION << ")");
        return false;
    }
    
    return true;
}

std::unique_ptr<MeasurementSpot> SpotPersistence::loadSpotFromJson(const nlohmann::json& spot_json) {
    try {
        auto spot = std::make_unique<MeasurementSpot>();
        spot->from_json(spot_json);
        
        // Validate loaded spot
        if (!spot->validate()) {
            LOG_WARN("Invalid spot configuration in persistence file, skipping");
            return nullptr;
        }
        
        return spot;
        
    } catch (const std::exception& e) {
        LOG_WARN("Failed to load spot from JSON: " << e.what() << ", skipping");
        return nullptr;
    }
}

nlohmann::json SpotPersistence::spotToJson(const MeasurementSpot& spot) {
    nlohmann::json spot_json = spot.to_json();
    
    // Add RPC-specific metadata
    spot_json["createdAt"] = getCurrentTimestamp();
    spot_json["lastReading"] = getCurrentTimestamp();
    spot_json["status"] = (spot.get_state() == SpotState::ACTIVE) ? "active" : "inactive";
    
    return spot_json;
}

std::string SpotPersistence::generateBackupPath() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << file_path_ << ".backup." << std::put_time(std::gmtime(&time_t), "%Y%m%d_%H%M%S");
    
    return ss.str();
}

std::string SpotPersistence::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    
    return ss.str();
}

} // namespace thermal