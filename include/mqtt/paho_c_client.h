#pragma once

#include <MQTTAsync.h>
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
    
    /**
     * @brief Called when a message is received
     * @param topic Topic the message was received on
     * @param payload Message payload
     */
    virtual void on_message_received(const std::string& topic, const std::string& payload) = 0;
};

/**
 * @brief Eclipse Paho MQTT C client wrapper
 * 
 * This class wraps the Eclipse Paho MQTT C async client to provide
 * a simplified C++ interface for ThingsBoard communication.
 */
class PahoCClient {
private:
    MQTTAsync client_;
    MQTTClientStats stats_;
    MQTTEventCallback* event_callback_;
    std::string server_uri_;
    std::string client_id_;
    
public:
    /**
     * @brief Construct MQTT client
     * @param server_uri MQTT broker URI (e.g., "tcp://localhost:1883")
     * @param client_id Unique client identifier
     * @param callback Event callback handler (optional)
     */
    explicit PahoCClient(const std::string& server_uri, 
                        const std::string& client_id,
                        MQTTEventCallback* callback = nullptr);
    
    /**
     * @brief Destructor - ensures clean disconnection
     */
    ~PahoCClient();
    
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
                bool clean_session = 1);
    
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
     * @brief Subscribe to topic
     * @param topic MQTT topic (can include wildcards)
     * @param qos Quality of Service level (0, 1, or 2)
     * @return true if subscription was initiated successfully
     */
    bool subscribe(const std::string& topic, int qos = 1);
    
    /**
     * @brief Unsubscribe from topic
     * @param topic MQTT topic
     * @return true if unsubscription was initiated successfully
     */
    bool unsubscribe(const std::string& topic);
    
    /**
     * @brief Get current client statistics
     * @return Current statistics
     */
    const MQTTClientStats& get_stats() const;
    
    /**
     * @brief Set event callback handler
     * @param callback Event callback (can be nullptr)
     */
    void set_event_callback(MQTTEventCallback* callback);
    
    // Static callback functions for Paho C library
    static void on_connection_lost_wrapper(void* context, char* cause);
    static void on_message_delivered_wrapper(void* context, MQTTAsync_token token);
    static void on_connect_success_wrapper(void* context, MQTTAsync_successData* response);
    static void on_connect_failure_wrapper(void* context, MQTTAsync_failureData* response);
    static void on_disconnect_wrapper(void* context, MQTTAsync_successData* response);
    static int on_message_arrived_wrapper(void* context, char* topic_name, int topic_len, MQTTAsync_message* message);
    
private:
    void update_state(MQTTConnectionState new_state);
    void handle_connection_success();
    void handle_connection_failure(const std::string& error);
    void handle_message_delivered(int token);
};

} // namespace thermal