#include "thingsboard/mock_device.h"
#include "common/logger.h"
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>

namespace thermal {

MockThingsBoardDevice::MockThingsBoardDevice(const ThingsBoardConfig& config)
    : config_(config) {
    
    // Validate configuration
    if (!config_.validate()) {
        throw std::invalid_argument("Invalid ThingsBoard configuration");
    }
    
    // Create mock MQTT client
    std::string server_uri = build_server_uri();
    std::string client_id = build_client_id();
    
    mqtt_client_ = std::make_unique<MockMQTTClient>(server_uri, client_id, this);
    
    LOG_INFO("Mock ThingsBoard device initialized: " << config_.device_id << " -> " << server_uri);
}

bool MockThingsBoardDevice::connect() {
    if (!mqtt_client_) {
        LOG_ERROR("Mock MQTT client not initialized");
        return false;
    }
    
    if (is_connected()) {
        LOG_DEBUG("Already connected to ThingsBoard (simulated)");
        return true;
    }
    
    LOG_INFO("Connecting to ThingsBoard (simulated): " << config_.host << ":" << config_.port);
    
    // ThingsBoard uses access token as username, no password
    return mqtt_client_->connect(config_.access_token, "", 
                                config_.keep_alive_seconds, true);
}

bool MockThingsBoardDevice::disconnect() {
    if (!mqtt_client_) {
        return true;
    }
    
    LOG_INFO("Disconnecting from ThingsBoard (simulated)");
    return mqtt_client_->disconnect();
}

bool MockThingsBoardDevice::is_connected() const {
    return mqtt_client_ && mqtt_client_->is_connected();
}

bool MockThingsBoardDevice::send_telemetry(int spot_id, double temperature) {
    if (!is_connected()) {
        LOG_ERROR("Not connected to ThingsBoard (simulated)");
        return false;
    }
    
    if (!validate_temperature(temperature)) {
        LOG_WARN("Invalid temperature reading " << temperature << "°C from spot " << spot_id 
                << " (outside -100°C to 500°C range), skipping");
        return false;  // Skip invalid readings as per specification
    }
    
    std::string topic = build_telemetry_topic();
    std::string payload = build_telemetry_payload(spot_id, temperature);
    
    LOG_DEBUG("Sending telemetry (simulated): spot=" << spot_id << " temp=" << temperature << "°C");
    
    bool result = mqtt_client_->publish(topic, payload, config_.qos_level, false);
    
    if (result) {
        LOG_DEBUG("Telemetry message queued successfully (simulated)");
    } else {
        LOG_ERROR("Failed to queue telemetry message (simulated)");
    }
    
    return result;
}

bool MockThingsBoardDevice::send_telemetry(int spot_id, double temperature,
                                          std::chrono::time_point<std::chrono::system_clock> timestamp) {
    if (!is_connected()) {
        LOG_ERROR("Not connected to ThingsBoard (simulated)");
        return false;
    }
    
    if (!validate_temperature(temperature)) {
        LOG_WARN("Invalid temperature reading " << temperature << "°C from spot " << spot_id 
                << " (outside -100°C to 500°C range), skipping");
        return false;
    }
    
    std::string topic = build_telemetry_topic();
    std::string payload = build_telemetry_payload_with_timestamp(spot_id, temperature, timestamp);
    
    LOG_DEBUG("Sending timestamped telemetry (simulated): spot=" << spot_id << " temp=" << temperature 
             << "°C at " << format_timestamp(timestamp));
    
    bool result = mqtt_client_->publish(topic, payload, config_.qos_level, false);
    
    if (result) {
        LOG_DEBUG("Timestamped telemetry message queued successfully (simulated)");
    } else {
        LOG_ERROR("Failed to queue timestamped telemetry message (simulated)");
    }
    
    return result;
}

const MQTTClientState& MockThingsBoardDevice::get_connection_state() const {
    static MQTTClientState empty_state;
    if (!mqtt_client_) {
        return empty_state;
    }
    return mqtt_client_->get_state();
}

void MockThingsBoardDevice::set_auto_reconnect(bool enable) {
    if (mqtt_client_) {
        mqtt_client_->set_auto_reconnect(enable);
        LOG_INFO("Auto-reconnect " << (enable ? "enabled" : "disabled") << " (simulated)");
    }
}

void MockThingsBoardDevice::set_simulation_mode(bool simulate_failures, int failure_rate) {
    if (mqtt_client_) {
        mqtt_client_->set_simulation_mode(simulate_failures, failure_rate);
        LOG_INFO("Simulation mode configured: failures=" << simulate_failures 
                << " rate=" << failure_rate << "%");
    }
}

// MQTTClientCallback interface implementation
void MockThingsBoardDevice::on_connection_lost(const std::string& cause) {
    LOG_WARN("ThingsBoard connection lost (simulated): " << cause);
}

void MockThingsBoardDevice::on_message_delivered(const std::string& topic) {
    LOG_DEBUG("Telemetry message delivered (simulated) to: " << topic);
}

void MockThingsBoardDevice::on_connection_success() {
    LOG_INFO("Successfully connected to ThingsBoard (simulated)");
}

void MockThingsBoardDevice::on_connection_failure(const MQTTError& error) {
    LOG_ERROR("ThingsBoard connection failed (simulated): " << error.error_message 
             << " (code: " << error.error_code << ")");
}

void MockThingsBoardDevice::on_disconnected() {
    LOG_INFO("Disconnected from ThingsBoard (simulated)");
}

// Private helper methods
std::string MockThingsBoardDevice::build_server_uri() const {
    std::string protocol = config_.use_ssl ? "ssl" : "tcp";
    return protocol + "://" + config_.host + ":" + std::to_string(config_.port);
}

std::string MockThingsBoardDevice::build_client_id() const {
    return config_.device_id + "_client";
}

std::string MockThingsBoardDevice::build_telemetry_topic() const {
    return "v1/devices/me/telemetry";
}

std::string MockThingsBoardDevice::build_telemetry_payload(int spot_id, double temperature) const {
    // Individual message format as clarified in specification
    nlohmann::json payload;
    payload["spot"] = spot_id;
    payload["temperature"] = temperature;
    
    return payload.dump();
}

std::string MockThingsBoardDevice::build_telemetry_payload_with_timestamp(
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

bool MockThingsBoardDevice::validate_temperature(double temperature) const {
    // Temperature validation range: -100°C to 500°C
    return temperature >= -100.0 && temperature <= 500.0;
}

std::string MockThingsBoardDevice::format_timestamp(
    std::chrono::time_point<std::chrono::system_clock> timestamp) const {
    
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
    return ss.str();
}

} // namespace thermal