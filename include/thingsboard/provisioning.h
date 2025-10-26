#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <functional>
#include <nlohmann/json.hpp>
#include "config/provisioning.h"

namespace thingsboard {

/**
 * @brief Enumeration for provisioning error types
 */
enum class ProvisioningError {
    NONE = 0,
    NETWORK_ERROR,
    AUTH_ERROR,
    VALIDATION_ERROR,
    SERVER_ERROR,
    TIMEOUT_ERROR,
    CONFIG_ERROR
};

/**
 * @brief Enumeration for provisioning status states
 */
enum class ProvisioningStatus {
    IDLE = 0,
    DETECTING_FILES,
    LOADING_CONFIG,
    CONNECTING,
    SENDING_REQUEST,
    WAITING_RESPONSE,
    VALIDATING_RESPONSE,
    UPDATING_CONFIG,
    COMPLETED,
    FAILED_CONFIG,
    FAILED_CONNECTION,
    FAILED_TIMEOUT,
    FAILED_VALIDATION,
    FAILED_UPDATE
};

/**
 * @brief Represents a device provisioning request to ThingsBoard
 */
class ProvisioningRequest {
public:
    ProvisioningRequest(const std::string& device_name,
                       const std::string& device_type,
                       const std::string& provision_key,
                       const std::string& provision_secret);

    // Getters
    const std::string& getDeviceName() const { return device_name_; }
    const std::string& getDeviceType() const { return device_type_; }
    const std::string& getProvisionKey() const { return provision_key_; }
    const std::string& getProvisionSecret() const { return provision_secret_; }
    const std::chrono::system_clock::time_point& getCreatedAt() const { return created_at_; }

    // JSON serialization
    nlohmann::json toJson() const;

    // Validation
    bool isValid() const;

private:
    std::string device_name_;
    std::string device_type_;
    std::string provision_key_;
    std::string provision_secret_;
    std::chrono::system_clock::time_point created_at_;
};

/**
 * @brief Represents a device provisioning response from ThingsBoard
 */
class ProvisioningResponse {
public:
    ProvisioningResponse() = default;
    
    // Create from JSON response
    static std::unique_ptr<ProvisioningResponse> fromJson(const nlohmann::json& json);

    // Getters
    const std::string& getStatus() const { return status_; }
    const std::string& getDeviceId() const { return device_id_; }
    const std::string& getDeviceName() const { return device_name_; }
    const std::string& getAccessToken() const { return access_token_; }
    const std::string& getCredentialsType() const { return credentials_type_; }
    const std::string& getErrorMessage() const { return error_message_; }
    const std::string& getErrorCode() const { return error_code_; }
    const std::chrono::system_clock::time_point& getReceivedAt() const { return received_at_; }

    // Status checks
    bool isSuccess() const { return status_ == "SUCCESS"; }
    bool isFailure() const { return status_ == "FAILURE"; }

    // Validation
    bool isValid() const;

public:
    ProvisioningResponse(const std::string& status,
                        const std::string& device_id,
                        const std::string& device_name,
                        const std::string& access_token,
                        const std::string& credentials_type,
                        const std::string& error_message = "",
                        const std::string& error_code = "");

private:
    std::string status_;
    std::string device_id_;
    std::string device_name_;
    std::string access_token_;
    std::string credentials_type_;
    std::string error_message_;
    std::string error_code_;
    std::chrono::system_clock::time_point received_at_;
};

/**
 * @brief Simple result structure for provisioning operations
 */
struct ProvisioningResult {
    bool success;
    std::string error_message;
    ProvisioningResponse response;
    
    ProvisioningResult() : success(false) {}
};

/**
 * @brief Main provisioning client for automatic device registration
 */
class ProvisioningClient {
public:
    using ProgressCallback = std::function<void(ProvisioningStatus, const std::string&)>;
    using CompletionCallback = std::function<void(bool success, const std::string& message)>;

    ProvisioningClient();
    ~ProvisioningClient();

    // Main provisioning workflow
    bool provision(const config::ProvisioningCredentials& credentials,
                  ProgressCallback progress_callback = nullptr,
                  CompletionCallback completion_callback = nullptr);

    // Status and state management
    ProvisioningStatus getCurrentStatus() const { return current_status_; }
    const std::string& getLastError() const { return last_error_; }
    const std::string& getCorrelationId() const { return correlation_id_; }
    
    // Get provisioned device credentials
    const std::string& getLastDeviceName() const { return last_device_name_; }
    const std::string& getLastAccessToken() const { return last_access_token_; }

    // Configuration
    void setTimeout(std::chrono::seconds timeout) { timeout_ = timeout; }
    void setRetryAttempts(int attempts) { max_retry_attempts_ = attempts; }

private:
    // Internal state
    ProvisioningStatus current_status_;
    std::string last_error_;
    std::string correlation_id_;
    std::chrono::seconds timeout_;
    int max_retry_attempts_;
    int current_retry_attempt_;

    // Callbacks
    ProgressCallback progress_callback_;
    CompletionCallback completion_callback_;
    
    // Received credentials from provisioning
    std::string last_device_name_;
    std::string last_access_token_;

    // Internal methods
    void updateStatus(ProvisioningStatus status, const std::string& message = "");
    std::string generateDeviceName(const std::string& prefix = "thermal-camera");
    std::string generateCorrelationId();
    
    // MQTT operations
    bool connectToMqttBroker(const config::ProvisioningCredentials& credentials);
    bool sendProvisioningRequest(const ProvisioningRequest& request);
    bool waitForProvisioningResponse(std::unique_ptr<ProvisioningResponse>& response);
    void disconnectFromMqttBroker();

    // Response handling
    void onProvisioningResponse(const std::string& topic, const std::string& payload);
    
    // Error handling
    bool shouldRetry(ProvisioningError error) const;
    std::chrono::milliseconds getRetryDelay() const;
    void handleProvisioningError(ProvisioningError error, const std::string& message);

    // Validation
    bool validateProvisioningResponse(const ProvisioningResponse& response,
                                    const ProvisioningRequest& request) const;
};

/**
 * @brief Helper functions for provisioning operations
 */
namespace provisioning_utils {
    
    /**
     * @brief Generate a unique device name with thermal-camera prefix
     */
    std::string generateThermalCameraDeviceName();
    
    /**
     * @brief Validate device name format
     */
    bool isValidDeviceName(const std::string& device_name);
    
    /**
     * @brief Convert ProvisioningError to string
     */
    std::string provisioningErrorToString(ProvisioningError error);
    
    /**
     * @brief Convert ProvisioningStatus to string
     */
    std::string provisioningStatusToString(ProvisioningStatus status);
    
    /**
     * @brief Check if file exists
     */
    bool fileExists(const std::string& file_path);
    
    /**
     * @brief Rename file atomically
     */
    bool renameFile(const std::string& old_path, const std::string& new_path);
}

} // namespace thingsboard