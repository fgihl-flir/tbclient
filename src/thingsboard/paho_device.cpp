#include "thingsboard/device.h"
#include "mqtt/paho_c_client.h"
#include "common/logger.h"
#include "thermal/rpc/thermal_rpc_handler.h"
#include "thingsboard/rpc/rpc_parser.h"
#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>

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
    
    // Initialize RPC parser
    rpc_parser_ = std::make_unique<thermal::RPCParser>();
    
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
    bool result = mqtt_client_->connect(config_.access_token, "", 
                                       config_.keep_alive_seconds, true);
    
    // Note: RPC subscription will happen in on_connection_success() callback
    // when the async connection is actually established
    
    return result;
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

bool ThingsBoardDevice::send_rpc_response(const std::string& request_id, const std::string& response) {
    if (!is_connected()) {
        LOG_ERROR("Not connected to ThingsBoard for RPC response");
        return false;
    }
    
    std::string topic = build_rpc_response_topic(request_id);
    LOG_DEBUG("Sending RPC response to " << topic);
    
    bool result = mqtt_client_->publish(topic, response, 1, false);
    if (result) {
        LOG_DEBUG("RPC response sent successfully for request " << request_id);
    } else {
        LOG_ERROR("Failed to send RPC response for request " << request_id);
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
    
    // Now that we're connected, subscribe to RPC commands
    LOG_INFO("Subscribing to ThingsBoard RPC topic: v1/devices/me/rpc/request/+");
    
    // Try subscription in a separate thread to avoid blocking
    std::thread subscription_thread([this]() {
        bool subscription_result = mqtt_client_->subscribe("v1/devices/me/rpc/request/+", 1);
        if (subscription_result) {
            LOG_DEBUG("Successfully queued RPC subscription request");
        } else {
            LOG_ERROR("Failed to queue RPC subscription request");
        }
    });
    
    // Detach the thread so it can run independently
    subscription_thread.detach();
}

void ThingsBoardDevice::on_connection_failure(const std::string& error) {
    LOG_ERROR("Failed to connect to ThingsBoard: " << error);
}

void ThingsBoardDevice::on_disconnected() {
    LOG_INFO("Disconnected from ThingsBoard");
}

void ThingsBoardDevice::on_message_received(const std::string& topic, const std::string& payload) {
    LOG_DEBUG("Received MQTT message on topic: " << topic);
    
    // Check if this is an RPC command
    if (topic.find("v1/devices/me/rpc/request/") == 0) {
        LOG_INFO("Processing RPC command from topic: " << topic);
        handle_rpc_command(topic, payload);
    } else {
        LOG_DEBUG("Ignoring non-RPC message on topic: " << topic);
    }
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

std::string ThingsBoardDevice::build_rpc_response_topic(const std::string& request_id) const {
    return "v1/devices/me/rpc/response/" + request_id;
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

void ThingsBoardDevice::handle_rpc_command(const std::string& topic, const std::string& payload) {
    try {
        std::string request_id = extract_request_id(topic);
        if (request_id.empty()) {
            LOG_ERROR("Invalid RPC topic format: " << topic);
            return;
        }
        
        LOG_INFO("Processing RPC command with request ID: " << request_id);
        LOG_INFO("RPC command payload: " << payload);
        
        // Parse the RPC command
        auto rpc_command = rpc_parser_->parseCommand(request_id, payload);
        
        // Check if parsing was successful by validating the command
        std::string validation_error = thermal::RPCParser::validateCommand(rpc_command);
        if (!validation_error.empty()) {
            LOG_ERROR("Failed to parse RPC command: " << validation_error);
            
            // Send error response for invalid command format
            nlohmann::json error_response = {
                {"error", {
                    {"code", thermal::RPCErrorCodes::INVALID_JSON},
                    {"message", validation_error}
                }}
            };
            send_rpc_response(request_id, error_response.dump());
            return;
        }
        
        // Check if we have a thermal RPC handler and if it supports this method
        std::string method_str = thermal::RPCCommand::methodToString(rpc_command.method);
        LOG_INFO("Parsed RPC method: " << method_str);
        if (thermal_rpc_handler_ && thermal_rpc_handler_->isSupported(method_str)) {
            LOG_DEBUG("Routing RPC command to thermal handler: " << method_str);
            thermal_rpc_handler_->handleRPCCommand(request_id, rpc_command);
        } else {
            LOG_WARN("Unsupported RPC method: " << method_str);
            
            // Send method not found error
            nlohmann::json error_response = {
                {"error", {
                    {"code", thermal::RPCErrorCodes::UNKNOWN_METHOD},
                    {"message", "Unsupported RPC method: " + method_str}
                }}
            };
            send_rpc_response(request_id, error_response.dump());
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception handling RPC command: " << e.what());
        
        // Send internal error response
        nlohmann::json error_response = {
            {"error", {
                {"code", thermal::RPCErrorCodes::INTERNAL_ERROR},
                {"message", "Internal error processing RPC command"}
            }}
        };
        
        try {
            std::string request_id = extract_request_id(topic);
            if (!request_id.empty()) {
                send_rpc_response(request_id, error_response.dump());
            }
        } catch (...) {
            LOG_ERROR("Failed to send error response");
        }
    }
}

std::string ThingsBoardDevice::extract_request_id(const std::string& rpc_topic) const {
    // Topic format: v1/devices/me/rpc/request/{request_id}
    const std::string prefix = "v1/devices/me/rpc/request/";
    if (rpc_topic.find(prefix) != 0) {
        return "";
    }
    
    return rpc_topic.substr(prefix.length());
}

void ThingsBoardDevice::setThermalRPCHandler(std::shared_ptr<thermal::ThermalRPCHandler> handler) {
    thermal_rpc_handler_ = handler;
    
    if (thermal_rpc_handler_) {
        // Set up response callback to route responses back through MQTT
        thermal_rpc_handler_->setResponseCallback(
            [this](const std::string& request_id, const nlohmann::json& response) {
                // Use a separate thread to avoid blocking issues (same as subscription fix)
                std::thread response_thread([this, request_id, response]() {
                    try {
                        std::string response_str = response.dump();
                        this->send_rpc_response(request_id, response_str);
                    } catch (const std::exception& e) {
                        LOG_ERROR("Exception in RPC response thread: " << e.what());
                    } catch (...) {
                        LOG_ERROR("Unknown exception in RPC response thread");
                    }
                });
                
                // Detach the thread so it can run independently
                response_thread.detach();
            }
        );
        
        LOG_INFO("Thermal RPC handler configured and response callback set");
    } else {
        LOG_INFO("Thermal RPC handler disabled");
    }
}

// Factory function to create the appropriate device implementation
std::unique_ptr<ThingsBoardDevice> create_thingsboard_device(const ThingsBoardConfig& config) {
    return std::make_unique<ThingsBoardDevice>(config);
}

} // namespace thermal