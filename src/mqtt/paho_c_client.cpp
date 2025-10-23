#include "mqtt/paho_c_client.h"
#include "common/logger.h"
#include <stdexcept>
#include <cstring>

namespace thermal {

PahoCClient::PahoCClient(const std::string& server_uri, 
                        const std::string& client_id,
                        MQTTEventCallback* callback)
    : client_(nullptr)
    , event_callback_(callback)
    , server_uri_(server_uri)
    , client_id_(client_id) {
    
    int rc = MQTTAsync_create(&client_, server_uri.c_str(), client_id.c_str(),
                              MQTTCLIENT_PERSISTENCE_NONE, nullptr);
    
    if (rc != MQTTASYNC_SUCCESS) {
        throw std::runtime_error("Failed to create MQTT client: " + std::to_string(rc));
    }
    
    // Set callbacks
    MQTTAsync_setCallbacks(client_, this, on_connection_lost_wrapper, 
                          nullptr, on_message_delivered_wrapper);
    
    update_state(MQTTConnectionState::DISCONNECTED);
    
    LOG_INFO("Paho C MQTT client created: " << client_id << " -> " << server_uri);
}

PahoCClient::~PahoCClient() {
    if (client_) {
        if (is_connected()) {
            disconnect(1000);
        }
        MQTTAsync_destroy(&client_);
    }
}

bool PahoCClient::connect(const std::string& username,
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
    
    update_state(MQTTConnectionState::CONNECTING);
    stats_.connection_attempts++;
    
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = keep_alive_seconds;
    conn_opts.cleansession = clean_session ? 1 : 0;
    conn_opts.onSuccess = on_connect_success_wrapper;
    conn_opts.onFailure = on_connect_failure_wrapper;
    conn_opts.context = this;
    
    if (!username.empty()) {
        conn_opts.username = username.c_str();
        if (!password.empty()) {
            conn_opts.password = password.c_str();
        }
    }
    
    LOG_INFO("Connecting to MQTT broker: " << server_uri_ 
            << " (client: " << client_id_ << ", user: " << username << ")");
    
    int rc = MQTTAsync_connect(client_, &conn_opts);
    
    if (rc != MQTTASYNC_SUCCESS) {
        handle_connection_failure("Connect failed with return code: " + std::to_string(rc));
        return false;
    }
    
    return true;
}

bool PahoCClient::disconnect(int timeout_ms) {
    if (!client_ || !is_connected()) {
        update_state(MQTTConnectionState::DISCONNECTED);
        return true;
    }
    
    update_state(MQTTConnectionState::DISCONNECTING);
    
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    disc_opts.timeout = timeout_ms;
    disc_opts.onSuccess = on_disconnect_wrapper;
    disc_opts.context = this;
    
    LOG_INFO("Disconnecting from MQTT broker...");
    
    int rc = MQTTAsync_disconnect(client_, &disc_opts);
    
    if (rc != MQTTASYNC_SUCCESS) {
        LOG_ERROR("Disconnect failed with return code: " << rc);
        update_state(MQTTConnectionState::DISCONNECTED);
        return false;
    }
    
    return true;
}

bool PahoCClient::is_connected() const {
    return client_ && MQTTAsync_isConnected(client_) && 
           stats_.state == MQTTConnectionState::CONNECTED;
}

bool PahoCClient::publish(const std::string& topic,
                         const std::string& payload,
                         int qos,
                         bool retained) {
    
    if (!is_connected()) {
        LOG_ERROR("Cannot publish: not connected to MQTT broker");
        return false;
    }
    
    MQTTAsync_message message = MQTTAsync_message_initializer;
    message.payload = const_cast<char*>(payload.c_str());
    message.payloadlen = payload.length();
    message.qos = qos;
    message.retained = retained ? 1 : 0;
    
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.context = this;
    
    LOG_DEBUG("Publishing to topic '" << topic << "': " << payload);
    
    int rc = MQTTAsync_sendMessage(client_, topic.c_str(), &message, &opts);
    
    if (rc != MQTTASYNC_SUCCESS) {
        LOG_ERROR("Failed to publish to topic '" << topic << "': " << rc);
        return false;
    }
    
    stats_.messages_sent++;
    stats_.last_message_time = std::chrono::steady_clock::now();
    return true;
}

const MQTTClientStats& PahoCClient::get_stats() const {
    return stats_;
}

void PahoCClient::set_event_callback(MQTTEventCallback* callback) {
    event_callback_ = callback;
}

// Static callback implementations
void PahoCClient::on_connection_lost_wrapper(void* context, char* cause) {
    auto* client = static_cast<PahoCClient*>(context);
    std::string cause_str = cause ? cause : "Unknown";
    
    LOG_WARN("MQTT connection lost: " << cause_str);
    
    client->update_state(MQTTConnectionState::DISCONNECTED);
    client->stats_.last_error = cause_str;
    
    if (client->event_callback_) {
        client->event_callback_->on_connection_lost(cause_str);
    }
}

void PahoCClient::on_message_delivered_wrapper(void* context, MQTTAsync_token token) {
    auto* client = static_cast<PahoCClient*>(context);
    client->handle_message_delivered(token);
}

void PahoCClient::on_connect_success_wrapper(void* context, MQTTAsync_successData* response) {
    (void)response; // Unused parameter
    if (context) {
        static_cast<PahoCClient*>(context)->handle_connection_success();
    }
}

void PahoCClient::on_connect_failure_wrapper(void* context, MQTTAsync_failureData* response) {
    auto* client = static_cast<PahoCClient*>(context);
    std::string error = "Connection failed";
    if (response && response->message) {
        error += ": " + std::string(response->message);
    }
    if (response) {
        error += " (code: " + std::to_string(response->code) + ")";
    }
    client->handle_connection_failure(error);
}

void PahoCClient::on_disconnect_wrapper(void* context, MQTTAsync_successData* response) {
    (void)response; // Unused parameter
    if (context) {
        PahoCClient* client = static_cast<PahoCClient*>(context);
        client->update_state(MQTTConnectionState::DISCONNECTED);
        if (client->event_callback_) {
            client->event_callback_->on_disconnected();
        }
    }
}

// Private methods
void PahoCClient::update_state(MQTTConnectionState new_state) {
    stats_.state = new_state;
}

void PahoCClient::handle_connection_success() {
    update_state(MQTTConnectionState::CONNECTED);
    stats_.last_connect_time = std::chrono::steady_clock::now();
    
    LOG_INFO("Successfully connected to MQTT broker");
    
    if (event_callback_) {
        event_callback_->on_connection_success();
    }
}

void PahoCClient::handle_connection_failure(const std::string& error) {
    update_state(MQTTConnectionState::FAILED);
    stats_.connection_failures++;
    stats_.last_error = error;
    
    LOG_ERROR("MQTT connection failed: " << error);
    
    if (event_callback_) {
        event_callback_->on_connection_failure(error);
    }
}

void PahoCClient::handle_message_delivered(int token) {
    LOG_DEBUG("Message delivery confirmed (token: " << token << ")");
    
    if (event_callback_) {
        event_callback_->on_message_delivered("", token);
    }
}

} // namespace thermal