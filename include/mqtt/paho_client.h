#pragma once

#include <mqtt/async_client.h>
#include <mqtt/callback.h>
#include <mqtt/message.h>
#include <mqtt/token.h>
#include <string>
#include <memory>
#include <chrono>
#include <functional>

namespace thermal {

/**
 * @brief MQTT client state for monitoring
 */
enum class MQTTConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    FAILED
};

/**
 * @brief MQTT client statistics
 */
struct MQTTClientStats {
    MQTTConnectionState state = MQTTConnectionState::DISCONNECTED;
    std::chrono::time_point<std::chrono::steady_clock> last_connect_time;
    std::chrono::time_point<std::chrono::steady_clock> last_message_time;
    int connection_attempts = 0;
    int messages_sent = 0;
    int connection_failures = 0;
    std::string last_error;
    
    void reset() {
        connection_attempts = 0;
        messages_sent = 0;
        connection_failures = 0;
        last_error.clear();
    }
};

/**
 * @brief Callback interface for MQTT events
 */
class MQTTEventCallback {
public:
    virtual ~MQTTEventCallback() = default;
    
    virtual void on_connection_lost(const std::string& cause) = 0;
    virtual void on_message_delivered(const std::string& topic, int message_id) = 0;
    virtual void on_connection_success() = 0;
    virtual void on_connection_failure(const std::string& error) = 0;
    virtual void on_disconnected() = 0;
};

/**
 * @brief Eclipse Paho MQTT C++ client wrapper
 * 
 * This class wraps the Eclipse Paho MQTT C++ async client to provide
 * a simplified interface for ThingsBoard communication with proper
 * error handling and connection management.
 */
class PahoMQTTClient : public mqtt::callback {
private:
    std::unique_ptr<mqtt::async_client> client_;
    MQTTClientStats stats_;
    MQTTEventCallback* event_callback_;
    bool auto_reconnect_;
    std::string server_uri_;
    std::string client_id_;
    
    // Connection options
    mqtt::connect_options conn_opts_;
    
public:
    /**
     * @brief Construct MQTT client
     * @param server_uri MQTT broker URI (e.g., "tcp://localhost:1883")
     * @param client_id Unique client identifier
     * @param callback Event callback handler (optional)
     */
    explicit PahoMQTTClient(const std::string& server_uri, 
                           const std::string& client_id,
                           MQTTEventCallback* callback = nullptr);
    
    /**
     * @brief Destructor - ensures clean disconnection
     */
    ~PahoMQTTClient();
    
    /**
     * @brief Connect to MQTT broker
     * @param username Username for authentication (optional)
     * @param password Password for authentication (optional)
     * @param keep_alive_seconds Keep alive interval
     * @param clean_session Whether to start a clean session
     * @return true if connection attempt was initiated successfully
     */
    bool connect(const std::string& username = "",
                const std::string& password = "",
                int keep_alive_seconds = 60,
                bool clean_session = true);
    
    /**
     * @brief Disconnect from MQTT broker
     * @param timeout_ms Timeout for disconnection
     * @return true if disconnection was successful
     */
    bool disconnect(int timeout_ms = 5000);
    
    /**
     * @brief Check if client is connected
     * @return true if connected to broker
     */
    bool is_connected() const;
    
    /**
     * @brief Publish message to topic
     * @param topic MQTT topic
     * @param payload Message payload
     * @param qos Quality of Service level (0, 1, or 2)
     * @param retained Whether message should be retained
     * @return true if publish was initiated successfully
     */
    bool publish(const std::string& topic,
                const std::string& payload,
                int qos = 1,
                bool retained = false);
    
    /**
     * @brief Get current client statistics
     * @return Current statistics
     */
    const MQTTClientStats& get_stats() const;
    
    /**
     * @brief Enable/disable automatic reconnection
     * @param enable Whether to enable auto-reconnect
     */
    void set_auto_reconnect(bool enable);
    
    /**
     * @brief Set event callback handler
     * @param callback Event callback (can be nullptr)
     */
    void set_event_callback(MQTTEventCallback* callback);
    
    // mqtt::callback interface implementation
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr token) override;
    
private:
    void update_state(MQTTConnectionState new_state);
    void handle_connection_success();
    void handle_connection_failure(const std::string& error);
};

} // namespace thermal