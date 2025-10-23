#pragma once

#include <string>
#include <functional>
#include <memory>
#include <chrono>

namespace thermal {

/**
 * @brief MQTT connection states
 */
enum class ConnectionState {
    DISCONNECTED,    // Not connected to broker
    CONNECTING,      // Connection attempt in progress
    CONNECTED,       // Successfully connected and authenticated
    RECONNECTING,    // Attempting to reconnect after failure
    FAILED          // Connection permanently failed (requires manual intervention)
};

/**
 * @brief MQTT client state tracking
 */
struct MQTTClientState {
    ConnectionState connection_state = ConnectionState::DISCONNECTED;
    std::chrono::time_point<std::chrono::steady_clock> last_connect_time;
    std::chrono::time_point<std::chrono::steady_clock> last_message_time;
    int reconnect_attempts = 0;
    int total_messages_sent = 0;
    int total_errors = 0;
    
    void reset_reconnect_attempts() { reconnect_attempts = 0; }
    void increment_reconnect_attempts() { reconnect_attempts++; }
    void increment_messages_sent() { total_messages_sent++; }
    void increment_errors() { total_errors++; }
};

/**
 * @brief MQTT-specific error information
 */
struct MQTTError {
    int error_code;
    std::string error_message;
    bool retry_possible;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
    
    MQTTError(int code, const std::string& message, bool can_retry)
        : error_code(code), error_message(message), retry_possible(can_retry),
          timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Callback interface for MQTT client events
 */
class MQTTClientCallback {
public:
    virtual ~MQTTClientCallback() = default;
    
    virtual void on_connection_lost(const std::string& cause) = 0;
    virtual void on_message_delivered(const std::string& topic) = 0;
    virtual void on_connection_success() = 0;
    virtual void on_connection_failure(const MQTTError& error) = 0;
    virtual void on_disconnected() = 0;
};

/**
 * @brief Mock MQTT client for testing User Story 1 without external dependencies
 */
class MockMQTTClient {
public:
    /**
     * @brief Constructor
     * @param server_uri The MQTT broker URI (e.g., "tcp://localhost:1883")
     * @param client_id Unique client identifier
     * @param callback Callback interface for events (optional)
     */
    explicit MockMQTTClient(const std::string& server_uri, 
                           const std::string& client_id,
                           MQTTClientCallback* callback = nullptr);
    
    ~MockMQTTClient() = default;

    // Non-copyable, movable
    MockMQTTClient(const MockMQTTClient&) = delete;
    MockMQTTClient& operator=(const MockMQTTClient&) = delete;
    MockMQTTClient(MockMQTTClient&&) = default;
    MockMQTTClient& operator=(MockMQTTClient&&) = default;

    /**
     * @brief Connect to the MQTT broker (simulated)
     * @param username Username for authentication (can be empty)
     * @param password Password for authentication (can be empty)
     * @param keep_alive_seconds Keep alive interval
     * @param clean_session Whether to use clean session
     * @return true if connection initiated successfully
     */
    bool connect(const std::string& username = "",
                const std::string& password = "",
                int keep_alive_seconds = 60,
                bool clean_session = true);

    /**
     * @brief Disconnect from the MQTT broker (simulated)
     * @param timeout_ms Timeout for disconnect operation
     * @return true if disconnect initiated successfully
     */
    bool disconnect(int timeout_ms = 5000);

    /**
     * @brief Publish a message to a topic (simulated)
     * @param topic The topic to publish to
     * @param payload The message payload
     * @param qos Quality of Service level (0, 1, or 2)
     * @param retained Whether message should be retained
     * @return true if publish initiated successfully
     */
    bool publish(const std::string& topic,
                const std::string& payload,
                int qos = 1,
                bool retained = false);

    /**
     * @brief Check if client is connected (simulated)
     * @return true if connected
     */
    bool is_connected() const;

    /**
     * @brief Get current connection state
     * @return Current ConnectionState
     */
    ConnectionState get_connection_state() const;

    /**
     * @brief Get client state information
     * @return MQTTClientState with statistics
     */
    const MQTTClientState& get_state() const;

    /**
     * @brief Enable automatic reconnection (simulated)
     * @param enable Whether to enable auto-reconnect
     * @param initial_delay_ms Initial delay before first reconnect attempt
     * @param max_delay_ms Maximum delay between reconnect attempts
     * @param max_attempts Maximum number of reconnect attempts (0 = unlimited)
     */
    void set_auto_reconnect(bool enable,
                           int initial_delay_ms = 1000,
                           int max_delay_ms = 30000,
                           int max_attempts = 0);

    /**
     * @brief Get the last error that occurred
     * @return Optional MQTTError if an error occurred
     */
    const std::shared_ptr<MQTTError>& get_last_error() const;

    /**
     * @brief Set simulation mode for testing
     * @param simulate_failures If true, simulate connection failures
     * @param failure_rate Percentage of operations that should fail (0-100)
     */
    void set_simulation_mode(bool simulate_failures, int failure_rate = 10);

private:
    std::string server_uri_;
    std::string client_id_;
    MQTTClientCallback* external_callback_;
    
    MQTTClientState state_;
    std::shared_ptr<MQTTError> last_error_;
    
    // Auto-reconnect settings
    bool auto_reconnect_enabled_ = false;
    int initial_delay_ms_ = 1000;
    int max_delay_ms_ = 30000;
    int max_attempts_ = 0;
    
    // Simulation settings
    bool simulate_failures_ = false;
    int failure_rate_ = 10;
    
    void update_state(ConnectionState new_state);
    bool should_simulate_failure() const;
    void simulate_connection_success();
    void simulate_connection_failure();
};

} // namespace thermal