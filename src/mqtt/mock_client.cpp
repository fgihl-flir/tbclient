#include "mqtt/mock_client.h"
#include "common/logger.h"
#include <thread>
#include <random>

namespace thermal {

MockMQTTClient::MockMQTTClient(const std::string& server_uri, 
                              const std::string& client_id,
                              MQTTClientCallback* callback)
    : server_uri_(server_uri), client_id_(client_id), external_callback_(callback) {
    
    update_state(ConnectionState::DISCONNECTED);
    LOG_INFO("Mock MQTT client created: " << client_id_ << " -> " << server_uri_);
}

bool MockMQTTClient::connect(const std::string& username,
                            const std::string& /* password */,
                            int /* keep_alive_seconds */,
                            bool /* clean_session */) {
    
    LOG_INFO("Mock MQTT connecting to: " << server_uri_ << " (user: " << username << ")");
    
    if (state_.connection_state == ConnectionState::CONNECTED) {
        return true;  // Already connected
    }

    update_state(ConnectionState::CONNECTING);
    
    // Simulate connection delay
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (should_simulate_failure()) {
            simulate_connection_failure();
        } else {
            simulate_connection_success();
        }
    }).detach();
    
    return true;  // Connection attempt initiated
}

bool MockMQTTClient::disconnect(int /* timeout_ms */) {
    if (state_.connection_state == ConnectionState::DISCONNECTED) {
        return true;  // Already disconnected
    }

    LOG_INFO("Mock MQTT disconnecting...");
    
    update_state(ConnectionState::DISCONNECTED);
    
    if (external_callback_) {
        external_callback_->on_disconnected();
    }
    
    return true;
}

bool MockMQTTClient::publish(const std::string& topic,
                            const std::string& message,
                            int /* qos */,
                            bool /* retained */) {
    
    if (!is_connected()) {
        last_error_ = std::make_shared<MQTTError>(-1, "Not connected to broker", true);
        state_.increment_errors();
        LOG_ERROR("Mock MQTT publish failed: not connected");
        return false;
    }

    if (should_simulate_failure()) {
        last_error_ = std::make_shared<MQTTError>(-1, "Simulated publish failure", true);
        state_.increment_errors();
        LOG_ERROR("Mock MQTT publish failed: simulated failure");
        return false;
    }
    
    LOG_INFO("Mock MQTT published to " << topic << ": " << message);
    
    state_.increment_messages_sent();
    state_.last_message_time = std::chrono::steady_clock::now();
    
    // Simulate delivery confirmation
    if (external_callback_) {
        std::thread([this, topic]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            external_callback_->on_message_delivered(topic);
        }).detach();
    }
    
    return true;
}

bool MockMQTTClient::is_connected() const {
    return state_.connection_state == ConnectionState::CONNECTED;
}

ConnectionState MockMQTTClient::get_connection_state() const {
    return state_.connection_state;
}

const MQTTClientState& MockMQTTClient::get_state() const {
    return state_;
}

void MockMQTTClient::set_auto_reconnect(bool enable,
                                       int initial_delay_ms,
                                       int max_delay_ms,
                                       int max_attempts) {
    auto_reconnect_enabled_ = enable;
    initial_delay_ms_ = initial_delay_ms;
    max_delay_ms_ = max_delay_ms;
    max_attempts_ = max_attempts;
    
    LOG_INFO("Mock MQTT auto-reconnect " << (enable ? "enabled" : "disabled"));
}

const std::shared_ptr<MQTTError>& MockMQTTClient::get_last_error() const {
    return last_error_;
}

void MockMQTTClient::set_simulation_mode(bool simulate_failures, int failure_rate) {
    simulate_failures_ = simulate_failures;
    failure_rate_ = failure_rate;
    
    LOG_INFO("Mock MQTT simulation mode: failures=" << simulate_failures 
             << " rate=" << failure_rate << "%");
}

void MockMQTTClient::update_state(ConnectionState new_state) {
    state_.connection_state = new_state;
}

bool MockMQTTClient::should_simulate_failure() const {
    if (!simulate_failures_) {
        return false;
    }
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    return dis(gen) <= failure_rate_;
}

void MockMQTTClient::simulate_connection_success() {
    update_state(ConnectionState::CONNECTED);
    state_.last_connect_time = std::chrono::steady_clock::now();
    state_.reset_reconnect_attempts();
    
    LOG_INFO("Mock MQTT connection successful");
    
    if (external_callback_) {
        external_callback_->on_connection_success();
    }
}

void MockMQTTClient::simulate_connection_failure() {
    update_state(ConnectionState::FAILED);
    
    MQTTError error(-1, "Simulated connection failure", true);
    last_error_ = std::make_shared<MQTTError>(error);
    
    LOG_ERROR("Mock MQTT connection failed: simulated failure");
    
    if (external_callback_) {
        external_callback_->on_connection_failure(error);
    }
}

} // namespace thermal