#pragma once

#include "mqtt/paho_c_client.h"
#include "config/configuration.h"
#include <memory>
#include <chrono>

namespace thermal {

/**
 * @brief ThingsBoard device client using real Paho MQTT
 * 
 * This class implements the ThingsBoard device protocol over MQTT
 * using the Eclipse Paho MQTT C++ library for production use.
 */
class ThingsBoardDevice : public MQTTEventCallback {
private:
    ThingsBoardConfig config_;
    std::unique_ptr<PahoCClient> mqtt_client_;
    
public:
    /**
     * @brief Construct ThingsBoard device
     * @param config ThingsBoard configuration
     */
    explicit ThingsBoardDevice(const ThingsBoardConfig& config);
    
    /**
     * @brief Destructor
     */
    ~ThingsBoardDevice();
    
    /**
     * @brief Connect to ThingsBoard
     * @return true if connection was successful
     */
    bool connect();
    
    /**
     * @brief Disconnect from ThingsBoard
     * @return true if disconnection was successful
     */
    bool disconnect();
    
    /**
     * @brief Check if connected to ThingsBoard
     * @return true if connected
     */
    bool is_connected() const;
    
    /**
     * @brief Send telemetry data for a measurement spot
     * @param spot_id Measurement spot identifier
     * @param temperature Temperature reading in Celsius
     * @return true if telemetry was sent successfully
     */
    bool send_telemetry(int spot_id, double temperature);
    
    /**
     * @brief Send telemetry data with timestamp
     * @param spot_id Measurement spot identifier
     * @param temperature Temperature reading in Celsius
     * @param timestamp Timestamp for the reading
     * @return true if telemetry was sent successfully
     */
    bool send_telemetry(int spot_id, double temperature,
                       std::chrono::time_point<std::chrono::system_clock> timestamp);
    
    /**
     * @brief Get MQTT client statistics
     * @return Current MQTT statistics
     */
    const MQTTClientStats& get_connection_stats() const;
    
    /**
     * @brief Enable/disable automatic reconnection
     * @param enable Whether to enable auto-reconnect
     */
    void set_auto_reconnect(bool enable);
    
    // MQTTEventCallback interface
    void on_connection_lost(const std::string& cause) override;
    void on_message_delivered(const std::string& topic, int message_id) override;
    void on_connection_success() override;
    void on_connection_failure(const std::string& error) override;
    void on_disconnected() override;
    
private:
    std::string build_server_uri() const;
    std::string build_client_id() const;
    std::string build_telemetry_topic() const;
    std::string build_telemetry_payload(int spot_id, double temperature) const;
    std::string build_telemetry_payload_with_timestamp(
        int spot_id, double temperature,
        std::chrono::time_point<std::chrono::system_clock> timestamp) const;
    bool validate_temperature(double temperature) const;
    std::string format_timestamp(
        std::chrono::time_point<std::chrono::system_clock> timestamp) const;
};

} // namespace thermal