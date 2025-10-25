#include "thingsboard/provisioning.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <MQTTAsync.h>

// Global variables for callback handling (not ideal but needed for C API)
static bool provisioning_completed = false;
static bool provisioning_success = false;
static std::string provisioning_result;
static std::string received_access_token;
static std::string received_device_name;
static std::string request_device_name;  // Store the device name from request

// MQTT callback for connection
void on_provisioning_connect(void* context, MQTTAsync_successData* /*response*/) {
    MQTTAsync client = (MQTTAsync)context;
    
    printf("[Provisioning client] Connected to ThingsBoard\n");
    
    // Subscribe to response topic
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc = MQTTAsync_subscribe(client, "/provision/response", 1, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("[Provisioning client] Failed to subscribe to response topic: %d\n", rc);
        provisioning_completed = true;
        provisioning_success = false;
        provisioning_result = "Failed to subscribe to response topic";
    }
}

// MQTT callback for connection failure
void on_provisioning_connect_failure(void* /*context*/, MQTTAsync_failureData* response) {
    printf("[Provisioning client] Connect failed, rc: %d\n", response ? response->code : 0);
    provisioning_completed = true;
    provisioning_success = false;
    provisioning_result = "Connection failed";
}

// MQTT callback for message arrival
int on_provisioning_message_arrived(void* /*context*/, char* topicName, int /*topicLen*/, MQTTAsync_message* message) {
    if (strcmp(topicName, "/provision/response") == 0) {
        std::string payload((char*)message->payload, message->payloadlen);
        printf("[Provisioning client] Received data from ThingsBoard: %s\n", payload.c_str());
        
        try {
            auto json_response = nlohmann::json::parse(payload);
            auto response = thingsboard::ProvisioningResponse::fromJson(json_response);
            
            if (response && response->getStatus() == "SUCCESS") {
                received_access_token = response->getAccessToken();
                // Use the device name from request since response might not include it
                received_device_name = response->getDeviceName().empty() ? 
                                     request_device_name : response->getDeviceName();
                provisioning_success = true;
                provisioning_result = "Provisioning successful";
                printf("[Provisioning client] Provisioning successful, access token: %s, device: %s\n", 
                       received_access_token.c_str(), received_device_name.c_str());
            } else {
                provisioning_success = false;
                if (response) {
                    provisioning_result = "Provisioning failed: " + response->getErrorMessage();
                    printf("[Provisioning client] Provisioning was unsuccessful: %s\n", response->getErrorMessage().c_str());
                } else {
                    provisioning_result = "Failed to parse response";
                    printf("[Provisioning client] Failed to parse response\n");
                }
            }
        } catch (const std::exception& e) {
            provisioning_success = false;
            provisioning_result = "JSON parse error: " + std::string(e.what());
            printf("[Provisioning client] JSON parse error: %s\n", e.what());
        }
        
        provisioning_completed = true;
    }
    
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

namespace thingsboard {

// ProvisioningRequest Implementation  
ProvisioningRequest::ProvisioningRequest(const std::string& device_name,
                                       const std::string& device_type,
                                       const std::string& provision_key,
                                       const std::string& provision_secret)
    : device_name_(device_name), device_type_(device_type), 
      provision_key_(provision_key), provision_secret_(provision_secret),
      created_at_(std::chrono::system_clock::now()) {
    
    if (device_name_.empty()) {
        throw std::invalid_argument("Device name cannot be empty");
    }
    if (device_type_.empty()) {
        throw std::invalid_argument("Device type cannot be empty"); 
    }
    if (provision_key_.empty()) {
        throw std::invalid_argument("Provision key cannot be empty");
    }
    if (provision_secret_.empty()) {
        throw std::invalid_argument("Provision secret cannot be empty");
    }
}

nlohmann::json ProvisioningRequest::toJson() const {
    nlohmann::json j;
    j["deviceName"] = device_name_;
    j["deviceType"] = device_type_;
    j["provisionDeviceKey"] = provision_key_;
    j["provisionDeviceSecret"] = provision_secret_;
    return j;
}

bool ProvisioningRequest::isValid() const {
    return !device_name_.empty() && !device_type_.empty() && 
           !provision_key_.empty() && !provision_secret_.empty();
}

// ProvisioningResponse Implementation
ProvisioningResponse::ProvisioningResponse(const std::string& status,
                                         const std::string& device_id,
                                         const std::string& device_name,
                                         const std::string& access_token,
                                         const std::string& credentials_type,
                                         const std::string& error_message,
                                         const std::string& error_code)
    : status_(status), device_id_(device_id), device_name_(device_name),
      access_token_(access_token), credentials_type_(credentials_type),
      error_message_(error_message), error_code_(error_code),
      received_at_(std::chrono::system_clock::now()) {}

std::unique_ptr<ProvisioningResponse> ProvisioningResponse::fromJson(const nlohmann::json& j) {
    std::string status, device_id, device_name, access_token, credentials_type, error_message, error_code;
    
    // Parse status
    if (j.contains("status") && j["status"].is_string()) {
        status = j["status"].get<std::string>();
    }
    
    // Parse success response fields
    if (status == "SUCCESS") {
        if (j.contains("credentialsType") && j["credentialsType"].is_string()) {
            credentials_type = j["credentialsType"].get<std::string>();
        }
        
        // ThingsBoard returns the access token in "credentialsValue" field
        if (j.contains("credentialsValue") && j["credentialsValue"].is_string()) {
            access_token = j["credentialsValue"].get<std::string>();
        }
        
        if (j.contains("deviceId") && j["deviceId"].is_string()) {
            device_id = j["deviceId"].get<std::string>();
        }
        
        if (j.contains("deviceName") && j["deviceName"].is_string()) {
            device_name = j["deviceName"].get<std::string>();
        }
    }
    
    // Parse error response fields
    if (j.contains("errorMsg") && j["errorMsg"].is_string()) {
        error_message = j["errorMsg"].get<std::string>();
    }
    
    if (j.contains("errorCode") && j["errorCode"].is_string()) {
        error_code = j["errorCode"].get<std::string>();
    }
    
    return std::make_unique<ProvisioningResponse>(status, device_id, device_name, 
                                                 access_token, credentials_type, 
                                                 error_message, error_code);
}

bool ProvisioningResponse::isValid() const {
    if (status_ == "SUCCESS") {
        return !access_token_.empty() && !credentials_type_.empty();
    } else if (status_ == "FAILURE") {
        return !error_message_.empty();
    }
    return false;
}

// ProvisioningClient Implementation
ProvisioningClient::ProvisioningClient() 
    : current_status_(ProvisioningStatus::IDLE),
      timeout_(std::chrono::seconds(30)),
      max_retry_attempts_(3),
      current_retry_attempt_(0) {}

ProvisioningClient::~ProvisioningClient() {
    // Cleanup if needed
}

bool ProvisioningClient::provision(const config::ProvisioningCredentials& credentials,
                                 ProgressCallback progress_callback,
                                 CompletionCallback completion_callback) {
    
    current_status_ = ProvisioningStatus::DETECTING_FILES;
    if (progress_callback) {
        progress_callback(current_status_, "Starting provisioning process");
    }
    
    try {
        // Reset global state
        provisioning_completed = false;
        provisioning_success = false;
        provisioning_result.clear();
        received_access_token.clear();
        received_device_name.clear();
        
        // Generate device name
        std::string device_name = thingsboard::provisioning_utils::generateThermalCameraDeviceName();
        request_device_name = device_name;  // Store for callback use
        
        current_status_ = ProvisioningStatus::CONNECTING;
        if (progress_callback) {
            progress_callback(current_status_, "Connecting to ThingsBoard provisioning service");
        }
        
        // MQTT connection parameters - use TCP as in Python example
        std::string server_uri = "tcp://" + credentials.getServerUrl() + ":" + std::to_string(credentials.getServerPort());
        std::string client_id = "provisioning-client-" + std::to_string(std::rand() % 10000);
        
        if (progress_callback) {
            progress_callback(current_status_, "Connecting to " + server_uri + " with client ID " + client_id);
        }
        
        // Create MQTT client
        MQTTAsync client;
        int rc = MQTTAsync_create(&client, server_uri.c_str(), client_id.c_str(), 
                                  MQTTCLIENT_PERSISTENCE_NONE, nullptr);
        if (rc != MQTTASYNC_SUCCESS) {
            throw std::runtime_error("Failed to create MQTT client for " + server_uri + 
                                   ": " + std::to_string(rc));
        }
        
        // Set callbacks
        MQTTAsync_setCallbacks(client, client, nullptr, on_provisioning_message_arrived, nullptr);
        
        // Set connection options with provisioning credentials
        // ThingsBoard provisioning uses "provision" as username and no password
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        conn_opts.keepAliveInterval = 60;
        conn_opts.cleansession = 1;
        conn_opts.username = "provision";
        conn_opts.password = nullptr;  // No password for provisioning connection
        conn_opts.connectTimeout = 30;
        conn_opts.onSuccess = on_provisioning_connect;
        conn_opts.onFailure = on_provisioning_connect_failure;
        conn_opts.context = client;
        
        // Connect to broker
        rc = MQTTAsync_connect(client, &conn_opts);
        if (rc != MQTTASYNC_SUCCESS) {
            MQTTAsync_destroy(&client);
            throw std::runtime_error("Failed to start MQTT connection: " + std::to_string(rc));
        }
        
        // Wait for connection and subscription
        int connection_wait = 0;
        while (!MQTTAsync_isConnected(client) && connection_wait < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            connection_wait++;
        }
        
        if (!MQTTAsync_isConnected(client)) {
            MQTTAsync_destroy(&client);
            throw std::runtime_error("MQTT connection failed or timed out");
        }
        
        // Wait a bit for subscription to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        current_status_ = ProvisioningStatus::SENDING_REQUEST;
        if (progress_callback) {
            progress_callback(current_status_, "Sending provisioning request");
        }
        
        // Create provisioning request message
        ProvisioningRequest request(device_name, "thermal-camera", 
                                  credentials.getProvisionDeviceKey(),
                                  credentials.getProvisionDeviceSecret());
        
        nlohmann::json request_json = request.toJson();
        std::string request_payload = request_json.dump();
        
        printf("[Provisioning client] Sending provisioning request: %s\n", request_payload.c_str());
        
        // Publish request
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
        pubmsg.payload = const_cast<char*>(request_payload.c_str());
        pubmsg.payloadlen = request_payload.length();
        pubmsg.qos = 1;
        pubmsg.retained = 0;
        
        rc = MQTTAsync_sendMessage(client, "/provision/request", &pubmsg, nullptr);
        if (rc != MQTTASYNC_SUCCESS) {
            MQTTAsync_disconnect(client, nullptr);
            MQTTAsync_destroy(&client);
            throw std::runtime_error("Failed to send provisioning request: " + std::to_string(rc));
        }
        
        if (progress_callback) {
            progress_callback(current_status_, "Waiting for provisioning response");
        }
        
        // Wait for response with timeout
        auto start_time = std::chrono::steady_clock::now();
        auto timeout_duration = std::chrono::seconds(30);
        
        while (!provisioning_completed && 
               (std::chrono::steady_clock::now() - start_time) < timeout_duration) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Cleanup MQTT connection
        MQTTAsync_disconnect(client, nullptr);
        MQTTAsync_destroy(&client);
        
        if (!provisioning_completed) {
            throw std::runtime_error("Provisioning response timeout");
        }
        
        if (!provisioning_success) {
            throw std::runtime_error("Provisioning failed: " + provisioning_result);
        }
        
        // Store the credentials for the workflow to use
        last_device_name_ = received_device_name;
        last_access_token_ = received_access_token;
        
        current_status_ = ProvisioningStatus::COMPLETED;
        if (progress_callback) {
            progress_callback(current_status_, "Device provisioning completed successfully");
        }
        
        if (completion_callback) {
            completion_callback(true, "Device " + received_device_name + " provisioned with token: " + received_access_token);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        current_status_ = ProvisioningStatus::FAILED_CONFIG;
        last_error_ = e.what();
        
        if (completion_callback) {
            completion_callback(false, last_error_);
        }
        
        return false;
    }
}

} // namespace thingsboard

// Utility Functions Implementation in provisioning_utils namespace
namespace thingsboard {
namespace provisioning_utils {

std::string generateThermalCameraDeviceName() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    return "thermal-camera-" + std::to_string(dis(gen));
}

bool isValidDeviceName(const std::string& device_name) {
    if (device_name.empty() || device_name.length() > 255) {
        return false;
    }
    
    // Check for valid characters (alphanumeric, hyphens, underscores)
    for (char c : device_name) {
        if (!std::isalnum(c) && c != '-' && c != '_') {
            return false;
        }
    }
    
    // Check for proper thermal camera naming pattern
    return device_name.find("thermal-camera-") == 0;
}

std::string provisioningErrorToString(ProvisioningError error) {
    switch (error) {
        case ProvisioningError::NONE: return "No error";
        case ProvisioningError::NETWORK_ERROR: return "Network error";
        case ProvisioningError::AUTH_ERROR: return "Authentication error";
        case ProvisioningError::VALIDATION_ERROR: return "Validation error";
        case ProvisioningError::SERVER_ERROR: return "Server error";
        case ProvisioningError::TIMEOUT_ERROR: return "Timeout error";
        case ProvisioningError::CONFIG_ERROR: return "Configuration error";
        default: return "Unknown error";
    }
}

} // namespace provisioning_utils
} // namespace thingsboard