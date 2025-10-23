#include "thingsboard/device.h"
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
    
    mqtt_client_ = std::make_unique<PahoMQTTClient>(server_uri, client_id, this);
    
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
        return false;  // Skip invalid readings as per specification
    }
    
    std::string topic = build_telemetry_topic();
    std::string payload = build_telemetry_payload(spot_id, temperature);
    
    LOG_DEBUG("Sending telemetry: spot=" << spot_id << " temp=" << temperature << "°C");
    
    bool result = mqtt_client_->publish(topic, payload, config_.qos_level, false);
    
    if (result) {
        LOG_DEBUG("Telemetry message sent successfully");
    } else {
        LOG_ERROR("Failed to send telemetry message");
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
    
    LOG_DEBUG("Sending timestamped telemetry: spot=" << spot_id << " temp=" << temperature 
             << "°C at " << format_timestamp(timestamp));
    
    bool result = mqtt_client_->publish(topic, payload, config_.qos_level, false);
    
    if (result) {
        LOG_DEBUG("Timestamped telemetry message sent successfully");
    } else {
        LOG_ERROR("Failed to send timestamped telemetry message");
    }
    
    return result;
}

const MQTTClientStats& ThingsBoardDevice::get_connection_stats() const {
    static MQTTClientStats empty_stats;
    if (!mqtt_client_) {
        return empty_stats;
    }
    return mqtt_client_->get_stats();
}

void ThingsBoardDevice::set_auto_reconnect(bool enable) {
    if (mqtt_client_) {
        mqtt_client_->set_auto_reconnect(enable);
        LOG_INFO("Auto-reconnect " << (enable ? "enabled" : "disabled"));
    }
}

// MQTTEventCallback interface implementation
void ThingsBoardDevice::on_connection_lost(const std::string& cause) {
    LOG_WARN("ThingsBoard connection lost: " << cause);
}

void ThingsBoardDevice::on_message_delivered(const std::string& topic, int message_id) {
    LOG_DEBUG("Telemetry message delivered to: " << topic << " (ID: " << message_id << ")");
}

void ThingsBoardDevice::on_connection_success() {
    LOG_INFO("Successfully connected to ThingsBoard");
}

void ThingsBoardDevice::on_connection_failure(const std::string& error) {
    LOG_ERROR("ThingsBoard connection failed: " << error);
}

void ThingsBoardDevice::on_disconnected() {
    LOG_INFO("Disconnected from ThingsBoard");
}

// Private helper methods
std::string ThingsBoardDevice::build_server_uri() const {
    std::string protocol = config_.use_ssl ? "ssl" : "tcp";
    return protocol + "://" + config_.host + ":" + std::to_string(config_.port);
}

std::string ThingsBoardDevice::build_client_id() const {
    return config_.device_id + "_client";
}

std::string ThingsBoardDevice::build_telemetry_topic() const {
    return "v1/devices/me/telemetry";
}

std::string ThingsBoardDevice::build_telemetry_payload(int spot_id, double temperature) const {
    // Individual message format as clarified in specification
    nlohmann::json payload;
    payload["spot"] = spot_id;
    payload["temperature"] = temperature;
    
    return payload.dump();
}

std::string ThingsBoardDevice::build_telemetry_payload_with_timestamp(
    int spot_id, double temperature,
    std::chrono::time_point<std::chrono::system_clock> timestamp) const {
    
    // ThingsBoard format with timestamp
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();
    
    nlohmann::json telemetry_data;
    telemetry_data["spot"] = spot_id;
    telemetry_data["temperature"] = temperature;
    
    nlohmann::json payload;
    payload["ts"] = timestamp_ms;
    payload["values"] = telemetry_data;
    
    return payload.dump();
}

bool ThingsBoardDevice::validate_temperature(double temperature) const {
    // Temperature validation range: -100°C to 500°C
    return temperature >= -100.0 && temperature <= 500.0;
}

std::string ThingsBoardDevice::format_timestamp(
    std::chrono::time_point<std::chrono::system_clock> timestamp) const {
    
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    return ss.str();
}

} // namespace thermal