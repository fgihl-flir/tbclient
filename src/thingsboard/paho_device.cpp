#include "thingsboard/device.h"
#include "mqtt/paho_c_client.h"
#include "common/logger.h"
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>

namespace thermal {

ThingsBoardDevice::ThingsBoardDevice(const ThingsBoardConfig& config)
    : config_(config) {
    
    // Validate configuration
    if (!config_.validate()) {
        throw std::invalid_argument("Invalid ThingsBoard configuration");
    }
    
    // Create MQTT client
    std::string server_uri = build_server_uri();
    std::string client_id = build_client_id();
    
    mqtt_client_ = std::make_unique<PahoCClient>(server_uri, client_id, this);
    
    LOG_INFO("ThingsBoard device initialized: " << config_.device_id << " -> " << server_uri);
}

ThingsBoardDevice::~ThingsBoardDevice() {
    if (mqtt_client_ && is_connected()) {
        disconnect();
    }
}

bool ThingsBoardDevice::connect() {
    if (!mqtt_client_) {
        LOG_ERROR("MQTT client not initialized");
        return false;
    }
    
    if (is_connected()) {
        LOG_DEBUG("Already connected to ThingsBoard");
        return true;
    }
    
    LOG_INFO("Connecting to ThingsBoard: " << config_.host << ":" << config_.port);
    
    // ThingsBoard uses access token as username, no password
    return mqtt_client_->connect(config_.access_token, "", 
                                config_.keep_alive_seconds, true);
}

bool ThingsBoardDevice::disconnect() {
    if (!mqtt_client_) {
        return true;
    }
    
    LOG_INFO("Disconnecting from ThingsBoard");
    return mqtt_client_->disconnect();
}

bool ThingsBoardDevice::is_connected() const {
    return mqtt_client_ && mqtt_client_->is_connected();
}

bool ThingsBoardDevice::send_telemetry(int spot_id, double temperature) {
    if (!is_connected()) {
        LOG_ERROR("Not connected to ThingsBoard");
        return false;
    }
    
    if (!validate_temperature(temperature)) {
        LOG_WARN("Invalid temperature reading " << temperature << "°C from spot " << spot_id 
                << " (outside -100°C to 500°C range), skipping");
        return false;
    }
    
    std::string topic = build_telemetry_topic();
    std::string payload = build_telemetry_payload(spot_id, temperature);
    
    LOG_DEBUG("Sending telemetry to " << topic << ": " << payload);
    
    bool result = mqtt_client_->publish(topic, payload, 1, false);
    if (result) {
        LOG_DEBUG("Telemetry sent successfully for spot " << spot_id 
                 << " (temperature: " << temperature << "°C)");
    } else {
        LOG_ERROR("Failed to send telemetry for spot " << spot_id);
    }
    
    return result;
}

bool ThingsBoardDevice::send_telemetry(int spot_id, double temperature,
                                     std::chrono::time_point<std::chrono::system_clock> timestamp) {
    if (!is_connected()) {
        LOG_ERROR("Not connected to ThingsBoard");
        return false;
    }
    
    if (!validate_temperature(temperature)) {
        LOG_WARN("Invalid temperature reading " << temperature << "°C from spot " << spot_id 
                << " (outside -100°C to 500°C range), skipping");
        return false;
    }
    
    std::string topic = build_telemetry_topic();
    std::string payload = build_telemetry_payload_with_timestamp(spot_id, temperature, timestamp);
    
    LOG_DEBUG("Sending timestamped telemetry to " << topic << ": " << payload);
    
    bool result = mqtt_client_->publish(topic, payload, 1, false);
    if (result) {
        LOG_DEBUG("Timestamped telemetry sent successfully for spot " << spot_id 
                 << " (temperature: " << temperature << "°C)");
    } else {
        LOG_ERROR("Failed to send timestamped telemetry for spot " << spot_id);
    }
    
    return result;
}

const MQTTClientStats& ThingsBoardDevice::get_connection_stats() const {
    if (!mqtt_client_) {
        static MQTTClientStats empty_stats;
        return empty_stats;
    }
    return mqtt_client_->get_stats();
}

void ThingsBoardDevice::set_auto_reconnect(bool enable) {
    // Auto-reconnect functionality not yet implemented in PahoCClient
    (void)enable; // Suppress unused parameter warning
    LOG_DEBUG("Auto-reconnect requested but not yet implemented");
}

// MQTTEventCallback interface implementation
void ThingsBoardDevice::on_connection_lost(const std::string& cause) {
    LOG_WARN("ThingsBoard connection lost: " << cause);
}

void ThingsBoardDevice::on_message_delivered(const std::string& topic, int message_id) {
    (void)topic; // Unused parameter
    LOG_DEBUG("Message delivered successfully (ID: " << message_id << ")");
}

void ThingsBoardDevice::on_connection_success() {
    LOG_INFO("Successfully connected to ThingsBoard");
}

void ThingsBoardDevice::on_connection_failure(const std::string& error) {
    LOG_ERROR("Failed to connect to ThingsBoard: " << error);
}

void ThingsBoardDevice::on_disconnected() {
    LOG_INFO("Disconnected from ThingsBoard");
}

// Private helper methods
std::string ThingsBoardDevice::build_server_uri() const {
    return (config_.use_ssl ? "ssl://" : "tcp://") + config_.host + ":" + std::to_string(config_.port);
}

std::string ThingsBoardDevice::build_client_id() const {
    return config_.device_id + "_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string ThingsBoardDevice::build_telemetry_topic() const {
    return "v1/devices/me/telemetry";
}

std::string ThingsBoardDevice::build_telemetry_payload(int spot_id, double temperature) const {
    nlohmann::json telemetry;
    std::string temp_key = "temperature_spot_" + std::to_string(spot_id);
    telemetry[temp_key] = temperature;
    return telemetry.dump();
}

std::string ThingsBoardDevice::build_telemetry_payload_with_timestamp(
    int spot_id, double temperature,
    std::chrono::time_point<std::chrono::system_clock> timestamp) const {
    
    nlohmann::json telemetry;
    std::string temp_key = "temperature_spot_" + std::to_string(spot_id);
    
    // Convert timestamp to milliseconds since epoch
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();
    
    nlohmann::json ts_data;
    ts_data["ts"] = timestamp_ms;
    ts_data["values"][temp_key] = temperature;
    
    return ts_data.dump();
}

bool ThingsBoardDevice::validate_temperature(double temperature) const {
    return temperature >= -100.0 && temperature <= 500.0;
}

std::string ThingsBoardDevice::format_timestamp(
    std::chrono::time_point<std::chrono::system_clock> timestamp) const {
    
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    return ss.str();
}

// Factory function to create the appropriate device implementation
std::unique_ptr<ThingsBoardDevice> create_thingsboard_device(const ThingsBoardConfig& config) {
    return std::make_unique<ThingsBoardDevice>(config);
}

} // namespace thermal