#include "mqtt/paho_client.h"
#include "common/logger.h"
#include <mqtt/connect_options.h>
#include <mqtt/create_options.h>
#include <stdexcept>

namespace thermal {

PahoMQTTClient::PahoMQTTClient(const std::string& server_uri, 
                              const std::string& client_id,
                              MQTTEventCallback* callback)
    : event_callback_(callback)
    , auto_reconnect_(false)
    , server_uri_(server_uri)
    , client_id_(client_id) {
    
    try {
        // Create MQTT client with create options
        mqtt::create_options create_opts;
        create_opts.set_mqtt_version(MQTTVERSION_3_1_1);
        
        client_ = std::make_unique<mqtt::async_client>(server_uri, client_id, create_opts);
        client_->set_callback(*this);
        
        // Set up connection options with defaults
        conn_opts_.set_keep_alive_interval(60);
        conn_opts_.set_clean_session(true);
        conn_opts_.set_automatic_reconnect(false); // We'll handle reconnection manually
        
        update_state(MQTTConnectionState::DISCONNECTED);
        
        LOG_INFO("Paho MQTT client created: " << client_id << " -> " << server_uri);
        
    } catch (const mqtt::exception& e) {
        LOG_ERROR("Failed to create MQTT client: " << e.what());
        throw std::runtime_error("MQTT client creation failed: " + std::string(e.what()));
    }
}

PahoMQTTClient::~PahoMQTTClient() {
    try {
        if (client_ && is_connected()) {
            disconnect(1000); // Quick disconnect on destruction
        }
    } catch (...) {
        // Ignore errors during destruction
    }
}

bool PahoMQTTClient::connect(const std::string& username,
                            const std::string& password,
                            int keep_alive_seconds,
                            bool clean_session) {
    
    if (!client_) {
        LOG_ERROR("MQTT client not initialized");
        return false;
    }
    
    if (is_connected()) {
        LOG_DEBUG("Already connected to MQTT broker");
        return true;
    }
    
    try {
        update_state(MQTTConnectionState::CONNECTING);
        stats_.connection_attempts++;
        
        // Configure connection options
        conn_opts_.set_keep_alive_interval(keep_alive_seconds);
        conn_opts_.set_clean_session(clean_session);
        conn_opts_.set_automatic_reconnect(auto_reconnect_);
        
        if (!username.empty()) {
            conn_opts_.set_user_name(username);
            if (!password.empty()) {
                conn_opts_.set_password(password);
            }
        }
        
        LOG_INFO("Connecting to MQTT broker: " << server_uri_ 
                << " (client: " << client_id_ << ", user: " << username << ")");
        
        // Asynchronous connection
        auto token = client_->connect(conn_opts_);
        
        // Wait for connection to complete (with timeout)
        if (token->wait_for(std::chrono::seconds(10))) {
            if (token->get_return_code() == MQTTASYNC_SUCCESS) {
                handle_connection_success();
                return true;
            } else {
                handle_connection_failure("Connection failed with return code: " + 
                                        std::to_string(token->get_return_code()));
                return false;
            }
        } else {
            handle_connection_failure("Connection timeout");
            return false;
        }
        
    } catch (const mqtt::exception& e) {
        handle_connection_failure("MQTT exception: " + std::string(e.what()));
        return false;
    } catch (const std::exception& e) {
        handle_connection_failure("Standard exception: " + std::string(e.what()));
        return false;
    }
}

bool PahoMQTTClient::disconnect(int timeout_ms) {
    if (!client_ || !is_connected()) {
        update_state(MQTTConnectionState::DISCONNECTED);
        return true;
    }
    
    try {
        update_state(MQTTConnectionState::DISCONNECTING);
        
        LOG_INFO("Disconnecting from MQTT broker...");
        
        auto token = client_->disconnect();
        
        // Wait for disconnection to complete
        if (token->wait_for(std::chrono::milliseconds(timeout_ms))) {
            update_state(MQTTConnectionState::DISCONNECTED);
            
            if (event_callback_) {
                event_callback_->on_disconnected();
            }
            
            LOG_INFO("Successfully disconnected from MQTT broker");
            return true;
        } else {
            LOG_WARN("Disconnect timeout, forcing disconnection");
            update_state(MQTTConnectionState::DISCONNECTED);
            return false;
        }
        
    } catch (const mqtt::exception& e) {
        LOG_ERROR("Error during disconnect: " << e.what());
        update_state(MQTTConnectionState::DISCONNECTED);
        return false;
    }
}

bool PahoMQTTClient::is_connected() const {
    return client_ && client_->is_connected() && 
           stats_.state == MQTTConnectionState::CONNECTED;
}

bool PahoMQTTClient::publish(const std::string& topic,
                            const std::string& payload,
                            int qos,
                            bool retained) {
    
    if (!is_connected()) {
        LOG_ERROR("Cannot publish: not connected to MQTT broker");
        return false;
    }
    
    try {
        auto message = mqtt::make_message(topic, payload);
        message->set_qos(qos);
        message->set_retained(retained);
        
        LOG_DEBUG("Publishing to topic '" << topic << "': " << payload);
        
        auto token = client_->publish(message);
        
        // For QoS 0, we don't wait. For QoS 1/2, we wait for confirmation
        if (qos > 0) {
            if (token->wait_for(std::chrono::seconds(5))) {
                stats_.messages_sent++;
                stats_.last_message_time = std::chrono::steady_clock::now();
                return true;
            } else {
                LOG_ERROR("Publish timeout for topic: " << topic);
                return false;
            }
        } else {
            // QoS 0 - fire and forget
            stats_.messages_sent++;
            stats_.last_message_time = std::chrono::steady_clock::now();
            return true;
        }
        
    } catch (const mqtt::exception& e) {
        LOG_ERROR("Failed to publish to topic '" << topic << "': " << e.what());
        return false;
    }
}

const MQTTClientStats& PahoMQTTClient::get_stats() const {
    return stats_;
}

void PahoMQTTClient::set_auto_reconnect(bool enable) {
    auto_reconnect_ = enable;
    
    if (client_) {
        conn_opts_.set_automatic_reconnect(enable);
        LOG_INFO("Auto-reconnect " << (enable ? "enabled" : "disabled"));
    }
}

void PahoMQTTClient::set_event_callback(MQTTEventCallback* callback) {
    event_callback_ = callback;
}

// mqtt::callback interface implementation
void PahoMQTTClient::connection_lost(const std::string& cause) {
    LOG_WARN("MQTT connection lost: " << cause);
    
    update_state(MQTTConnectionState::DISCONNECTED);
    stats_.last_error = cause;
    
    if (event_callback_) {
        event_callback_->on_connection_lost(cause);
    }
}

void PahoMQTTClient::message_arrived(mqtt::const_message_ptr msg) {
    // For telemetry client, we typically don't receive messages
    // This would be used for command/control in future versions
    LOG_DEBUG("Message arrived on topic: " << msg->get_topic());
}

void PahoMQTTClient::delivery_complete(mqtt::delivery_token_ptr token) {
    auto topic = token->get_message()->get_topic();
    LOG_DEBUG("Message delivery confirmed for topic: " << topic);
    
    if (event_callback_) {
        event_callback_->on_message_delivered(topic, token->get_message_id());
    }
}

// Private methods
void PahoMQTTClient::update_state(MQTTConnectionState new_state) {
    stats_.state = new_state;
}

void PahoMQTTClient::handle_connection_success() {
    update_state(MQTTConnectionState::CONNECTED);
    stats_.last_connect_time = std::chrono::steady_clock::now();
    
    LOG_INFO("Successfully connected to MQTT broker");
    
    if (event_callback_) {
        event_callback_->on_connection_success();
    }
}

void PahoMQTTClient::handle_connection_failure(const std::string& error) {
    update_state(MQTTConnectionState::FAILED);
    stats_.connection_failures++;
    stats_.last_error = error;
    
    LOG_ERROR("MQTT connection failed: " << error);
    
    if (event_callback_) {
        event_callback_->on_connection_failure(error);
    }
}

} // namespace thermal