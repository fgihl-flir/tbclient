#include "mqtt/client.h"
#include <iostream>
#include <thread>
#include <algorithm>
#include <cmath>

namespace thermal {

// Internal callback handler for Paho MQTT events
class MQTTClient::CallbackHandler : public virtual mqtt::callback {
public:
    explicit CallbackHandler(MQTTClient* client) : client_(client) {}

    void connection_lost(const std::string& cause) override {
        if (client_) {
            client_->handle_connection_lost(cause);
        }
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        // This client is publish-only, so we don't expect incoming messages
        // But we need to implement this for the interface
    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {
        if (client_ && client_->external_callback_) {
            try {
                auto topic = token->get_message()->get_topic();
                client_->external_callback_->on_message_delivered(topic);
            } catch (...) {
                // Ignore errors in callback to prevent cascade failures
            }
        }
    }

private:
    MQTTClient* client_;
};

// Action listener for async operations
class MQTTClient::ActionListener : public virtual mqtt::iaction_listener {
public:
    explicit ActionListener(MQTTClient* client, const std::string& action)
        : client_(client), action_(action) {}

    void on_success(const mqtt::token& token) override {
        if (client_ && action_ == "connect") {
            client_->handle_connection_success();
        }
    }

    void on_failure(const mqtt::token& token) override {
        if (client_ && action_ == "connect") {
            auto code = token.get_return_code();
            std::string message = "Connection failed with code: " + std::to_string(code);
            client_->handle_connection_failure(code, message);
        }
    }

private:
    MQTTClient* client_;
    std::string action_;
};

// MQTTClient implementation
MQTTClient::MQTTClient(const std::string& server_uri, 
                      const std::string& client_id,
                      MQTTClientCallback* callback)
    : external_callback_(callback) {
    
    try {
        client_ = std::make_unique<mqtt::async_client>(server_uri, client_id);
        callback_handler_ = std::make_unique<CallbackHandler>(this);
        client_->set_callback(*callback_handler_);
        
        update_state(ConnectionState::DISCONNECTED);
    } catch (const std::exception& e) {
        last_error_ = std::make_shared<MQTTError>(-1, 
            "Failed to create MQTT client: " + std::string(e.what()), false);
        update_state(ConnectionState::FAILED);
    }
}

MQTTClient::~MQTTClient() {
    if (client_ && is_connected()) {
        try {
            client_->disconnect()->wait_for(std::chrono::seconds(5));
        } catch (...) {
            // Ignore errors during cleanup
        }
    }
}

bool MQTTClient::connect(const std::string& username,
                        const std::string& password,
                        int keep_alive_seconds,
                        bool clean_session) {
    
    if (!client_) {
        last_error_ = std::make_shared<MQTTError>(-1, "MQTT client not initialized", false);
        return false;
    }

    if (state_.connection_state == ConnectionState::CONNECTED) {
        return true;  // Already connected
    }

    try {
        update_state(ConnectionState::CONNECTING);
        
        mqtt::connect_options conn_opts;
        conn_opts.set_keep_alive_interval(keep_alive_seconds);
        conn_opts.set_clean_session(clean_session);
        
        if (!username.empty()) {
            conn_opts.set_user_name(username);
            if (!password.empty()) {
                conn_opts.set_password(password);
            }
        }

        auto connect_listener = std::make_shared<ActionListener>(this, "connect");
        auto token = client_->connect(conn_opts, nullptr, *connect_listener);
        
        return true;  // Async operation initiated
        
    } catch (const std::exception& e) {
        handle_connection_failure(-1, "Connect exception: " + std::string(e.what()));
        return false;
    }
}

bool MQTTClient::disconnect(int timeout_ms) {
    if (!client_ || !is_connected()) {
        return true;  // Already disconnected
    }

    try {
        auto token = client_->disconnect();
        auto result = token->wait_for(std::chrono::milliseconds(timeout_ms));
        
        if (result) {
            update_state(ConnectionState::DISCONNECTED);
            if (external_callback_) {
                external_callback_->on_disconnected();
            }
            return true;
        } else {
            last_error_ = std::make_shared<MQTTError>(-1, "Disconnect timeout", false);
            return false;
        }
    } catch (const std::exception& e) {
        last_error_ = std::make_shared<MQTTError>(-1, 
            "Disconnect exception: " + std::string(e.what()), false);
        return false;
    }
}

bool MQTTClient::publish(const std::string& topic,
                        const std::string& payload,
                        int qos,
                        bool retained) {
    
    if (!is_connected()) {
        last_error_ = std::make_shared<MQTTError>(-1, "Not connected to broker", true);
        state_.increment_errors();
        return false;
    }

    try {
        auto msg = mqtt::make_message(topic, payload);
        msg->set_qos(qos);
        msg->set_retained(retained);
        
        auto token = client_->publish(msg);
        
        // For async client, we don't wait for completion here
        // Completion will be handled by delivery_complete callback
        state_.increment_messages_sent();
        state_.last_message_time = std::chrono::steady_clock::now();
        
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = std::make_shared<MQTTError>(-1, 
            "Publish exception: " + std::string(e.what()), true);
        state_.increment_errors();
        return false;
    }
}

bool MQTTClient::is_connected() const {
    return client_ && client_->is_connected() && 
           state_.connection_state == ConnectionState::CONNECTED;
}

ConnectionState MQTTClient::get_connection_state() const {
    return state_.connection_state;
}

const MQTTClientState& MQTTClient::get_state() const {
    return state_;
}

void MQTTClient::set_auto_reconnect(bool enable,
                                   int initial_delay_ms,
                                   int max_delay_ms,
                                   int max_attempts) {
    auto_reconnect_enabled_ = enable;
    initial_delay_ms_ = initial_delay_ms;
    max_delay_ms_ = max_delay_ms;
    max_attempts_ = max_attempts;
}

const std::shared_ptr<MQTTError>& MQTTClient::get_last_error() const {
    return last_error_;
}

void MQTTClient::handle_connection_lost(const std::string& cause) {
    last_error_ = std::make_shared<MQTTError>(-1, "Connection lost: " + cause, true);
    
    if (external_callback_) {
        external_callback_->on_connection_lost(cause);
    }

    if (auto_reconnect_enabled_) {
        update_state(ConnectionState::RECONNECTING);
        attempt_reconnect();
    } else {
        update_state(ConnectionState::DISCONNECTED);
    }
}

void MQTTClient::handle_connection_success() {
    update_state(ConnectionState::CONNECTED);
    state_.last_connect_time = std::chrono::steady_clock::now();
    state_.reset_reconnect_attempts();
    
    if (external_callback_) {
        external_callback_->on_connection_success();
    }
}

void MQTTClient::handle_connection_failure(int error_code, const std::string& message) {
    last_error_ = std::make_shared<MQTTError>(error_code, message, true);
    
    // Check for authentication failure (stop retrying as per spec)
    if (error_code == MQTTASYNC_BAD_USERNAME_OR_PASSWORD || 
        error_code == MQTTASYNC_NOT_AUTHORIZED) {
        update_state(ConnectionState::FAILED);
        if (external_callback_) {
            external_callback_->on_connection_failure(*last_error_);
        }
        return;
    }
    
    if (external_callback_) {
        external_callback_->on_connection_failure(*last_error_);
    }

    if (auto_reconnect_enabled_ && 
        (max_attempts_ == 0 || state_.reconnect_attempts < max_attempts_)) {
        update_state(ConnectionState::RECONNECTING);
        attempt_reconnect();
    } else {
        update_state(ConnectionState::FAILED);
    }
}

void MQTTClient::attempt_reconnect() {
    if (!auto_reconnect_enabled_ || 
        (max_attempts_ > 0 && state_.reconnect_attempts >= max_attempts_)) {
        update_state(ConnectionState::FAILED);
        return;
    }

    state_.increment_reconnect_attempts();
    
    int delay = calculate_reconnect_delay();
    
    // Use a separate thread for delayed reconnection
    std::thread([this, delay]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        
        if (state_.connection_state == ConnectionState::RECONNECTING) {
            // Attempt to reconnect with default parameters
            connect();
        }
    }).detach();
}

int MQTTClient::calculate_reconnect_delay() const {
    // Exponential backoff: delay = initial_delay * 2^(attempts-1)
    int delay = initial_delay_ms_ * static_cast<int>(std::pow(2, state_.reconnect_attempts - 1));
    return std::min(delay, max_delay_ms_);
}

void MQTTClient::update_state(ConnectionState new_state) {
    state_.connection_state = new_state;
}

} // namespace thermal