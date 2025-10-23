#pragma once

#include <string>
#include <memory>
#include <chrono>
#include "mqtt/client.h"
#include "config/configuration.h"

namespace thermal {

/**
 * @brief ThingsBoard device interface for MQTT communication
 */
class ThingsBoardDevice : public MQTTClientCallback {
public:
    /**
     * @brief Constructor
     * @param config ThingsBoard configuration
     */
    explicit ThingsBoardDevice(const ThingsBoardConfig& config);
    
    ~ThingsBoardDevice() override = default;

    // Non-copyable, movable
    ThingsBoardDevice(const ThingsBoardDevice&) = delete;
    ThingsBoardDevice& operator=(const ThingsBoardDevice&) = delete;
    ThingsBoardDevice(ThingsBoardDevice&&) = default;
    ThingsBoardDevice& operator=(ThingsBoardDevice&&) = default;

    /**
     * @brief Initialize and connect to ThingsBoard
     * @return true if connection initiated successfully
     */
    bool connect();

    /**
     * @brief Disconnect from ThingsBoard
     * @return true if disconnect successful
     */
    bool disconnect();

    /**
     * @brief Check if device is connected
     * @return true if connected
     */
    bool is_connected() const;

    /**
     * @brief Send telemetry data to ThingsBoard
     * @param spot_id Measurement spot identifier
     * @param temperature Temperature value in Celsius
     * @return true if message sent successfully
     */
    bool send_telemetry(int spot_id, double temperature);

    /**
     * @brief Send telemetry data with timestamp
     * @param spot_id Measurement spot identifier
     * @param temperature Temperature value in Celsius
     * @param timestamp When the measurement was taken
     * @return true if message sent successfully
     */
    bool send_telemetry(int spot_id, double temperature, 
                       std::chrono::time_point<std::chrono::system_clock> timestamp);

    /**
     * @brief Get device connection statistics
     * @return MQTTClientState with connection info
     */
    const MQTTClientState& get_connection_state() const;

    /**
     * @brief Enable auto-reconnection
     * @param enable Whether to enable auto-reconnect
     */
    void set_auto_reconnect(bool enable);

    // MQTTClientCallback interface
    void on_connection_lost(const std::string& cause) override;
    void on_message_delivered(const std::string& topic) override;
    void on_connection_success() override;
    void on_connection_failure(const MQTTError& error) override;
    void on_disconnected() override;

private:
    ThingsBoardConfig config_;
    std::unique_ptr<MQTTClient> mqtt_client_;
    
    std::string build_server_uri() const;
    std::string build_client_id() const;
    std::string build_telemetry_topic() const;
    std::string build_telemetry_payload(int spot_id, double temperature) const;
    std::string build_telemetry_payload_with_timestamp(int spot_id, double temperature,
                                                      std::chrono::time_point<std::chrono::system_clock> timestamp) const;
    
    bool validate_temperature(double temperature) const;
    std::string format_timestamp(std::chrono::time_point<std::chrono::system_clock> timestamp) const;
};

} // namespace thermal