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
    
    bool is_connected() const {
        return mqtt_client_ && mqtt_client_->is_connected();
    }
    
    bool send_telemetry(int spot_id, double temperature) {
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
        
        LOG_DEBUG("Sending telemetry: spot=" << spot_id << " temp=" << temperature << "°C");
        
        bool result = mqtt_client_->publish(topic, payload, config_.qos_level, false);
        
        if (result) {
            LOG_DEBUG("Telemetry message sent successfully");
        } else {
            LOG_ERROR("Failed to send telemetry message");
        }
        
        return result;
    }
    
    bool send_telemetry(int spot_id, double temperature,
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
    
    const MQTTClientStats& get_connection_stats() const {
        static MQTTClientStats empty_stats;
        if (!mqtt_client_) {
            return empty_stats;
        }
        return mqtt_client_->get_stats();
    }
    
    void set_auto_reconnect(bool enable) {
        // Paho C client doesn't have auto-reconnect built-in
        // Would need to implement manually if required
        LOG_INFO("Auto-reconnect " << (enable ? "enabled" : "disabled") << " (manual implementation required)");
    }
    
    // MQTTEventCallback interface implementation
    void on_connection_lost(const std::string& cause) override {
        LOG_WARN("ThingsBoard connection lost: " << cause);
    }
    
    void on_message_delivered(const std::string& topic, int message_id) override {
        LOG_DEBUG("Telemetry message delivered (ID: " << message_id << ")");
    }
    
    void on_connection_success() override {
        LOG_INFO("Successfully connected to ThingsBoard");
    }
    
    void on_connection_failure(const std::string& error) override {
        LOG_ERROR("ThingsBoard connection failed: " << error);
    }
    
    void on_disconnected() override {
        LOG_INFO("Disconnected from ThingsBoard");
    }
    
private:
    std::string build_server_uri() const {
        std::string protocol = config_.use_ssl ? "ssl" : "tcp";
        return protocol + "://" + config_.host + ":" + std::to_string(config_.port);
    }
    
    std::string build_client_id() const {
        return config_.device_id + "_client";
    }
    
    std::string build_telemetry_topic() const {
        return "v1/devices/me/telemetry";
    }
    
    std::string build_telemetry_payload(int spot_id, double temperature) const {
        nlohmann::json payload;
        payload["spot"] = spot_id;
        payload["temperature"] = temperature;
        return payload.dump();
    }
    
    std::string build_telemetry_payload_with_timestamp(
        int spot_id, double temperature,
        std::chrono::time_point<std::chrono::system_clock> timestamp) const {
        
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
    
    bool validate_temperature(double temperature) const {
        return temperature >= -100.0 && temperature <= 500.0;
    }
    
    std::string format_timestamp(
        std::chrono::time_point<std::chrono::system_clock> timestamp) const {
        
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S UTC");
        return ss.str();
    }
};

// Factory function to create the appropriate device implementation
std::unique_ptr<ThingsBoardDevice> create_thingsboard_device(const ThingsBoardConfig& config) {
    return std::make_unique<PahoThingsBoardDevice>(config);
}

} // namespace thermal