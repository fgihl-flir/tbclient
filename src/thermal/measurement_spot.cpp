#include "thermal/measurement_spot.h"
#include <stdexcept>
#include <random>
#include <regex>

namespace thermal {

bool MeasurementSpot::validate() const {
    if (id <= 0) {
        throw std::invalid_argument("Spot ID must be positive");
    }
    
    if (name.empty()) {
        throw std::invalid_argument("Spot name cannot be empty");
    }
    
    // Name validation: allow alphanumeric, spaces, underscores, hyphens
    std::regex name_pattern("^[a-zA-Z0-9 _-]+$");
    if (!std::regex_match(name, name_pattern)) {
        throw std::invalid_argument("Spot name contains invalid characters");
    }
    
    if (x < 0 || y < 0) {
        throw std::invalid_argument("Coordinates must be non-negative");
    }
    
    if (min_temp >= max_temp) {
        throw std::invalid_argument("Minimum temperature must be less than maximum temperature");
    }
    
    // Temperature range validation: -100°C to 500°C
    if (min_temp < -100.0 || max_temp > 500.0) {
        throw std::invalid_argument("Temperature range must be between -100°C and 500°C");
    }
    
    if (noise_factor < 0.0 || noise_factor > 1.0) {
        throw std::invalid_argument("Noise factor must be between 0.0 and 1.0");
    }
    
    return true;
}

void MeasurementSpot::from_json(const nlohmann::json& json_data) {
    if (json_data.contains("id")) {
        id = json_data["id"].get<int>();
    }
    if (json_data.contains("name")) {
        name = json_data["name"].get<std::string>();
    }
    if (json_data.contains("x")) {
        x = json_data["x"].get<int>();
    }
    if (json_data.contains("y")) {
        y = json_data["y"].get<int>();
    }
    if (json_data.contains("min_temp")) {
        min_temp = json_data["min_temp"].get<double>();
    }
    if (json_data.contains("max_temp")) {
        max_temp = json_data["max_temp"].get<double>();
    }
    if (json_data.contains("noise_factor")) {
        noise_factor = json_data["noise_factor"].get<double>();
    }
    if (json_data.contains("enabled")) {
        enabled = json_data["enabled"].get<bool>();
    }
    
    // Set initial state based on enabled flag
    set_state(enabled ? SpotState::ACTIVE : SpotState::INACTIVE);
}

nlohmann::json MeasurementSpot::to_json() const {
    return nlohmann::json{
        {"id", id},
        {"name", name},
        {"x", x},
        {"y", y},
        {"min_temp", min_temp},
        {"max_temp", max_temp},
        {"noise_factor", noise_factor},
        {"enabled", enabled}
    };
}

double MeasurementSpot::generate_temperature() const {
    if (!is_ready()) {
        throw std::runtime_error("Spot is not ready for measurement");
    }
    
    // Generate random temperature within the configured range
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // Base temperature: midpoint of range
    double base_temp = (min_temp + max_temp) / 2.0;
    
    // Temperature range for variation
    double temp_range = (max_temp - min_temp) * noise_factor;
    
    // Generate variation within noise factor
    std::uniform_real_distribution<double> dist(-temp_range / 2.0, temp_range / 2.0);
    double variation = dist(gen);
    
    double generated_temp = base_temp + variation;
    
    // Clamp to valid range
    generated_temp = std::max(min_temp, std::min(max_temp, generated_temp));
    
    return generated_temp;
}

bool MeasurementSpot::is_temperature_expected(double temperature) const {
    return temperature >= min_temp && temperature <= max_temp;
}

void MeasurementSpot::set_state(SpotState new_state) {
    state = new_state;
}

SpotState MeasurementSpot::get_state() const {
    return state;
}

bool MeasurementSpot::is_ready() const {
    return enabled && state == SpotState::ACTIVE;
}

} // namespace thermal