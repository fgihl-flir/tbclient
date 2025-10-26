#pragma once

#include <string>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>

namespace config {

/**
 * @brief Represents provisioning credentials loaded from provision.json
 */
class ProvisioningCredentials {
public:
    ProvisioningCredentials() = default;
    
    // Load from JSON file
    static std::unique_ptr<ProvisioningCredentials> loadFromFile(const std::string& file_path);
    
    // Load from JSON object
    static std::unique_ptr<ProvisioningCredentials> fromJson(const nlohmann::json& json);

    // Getters
    const std::string& getServerUrl() const { return server_url_; }
    int getServerPort() const { return server_port_; }
    const std::string& getProvisionDeviceKey() const { return provision_device_key_; }
    const std::string& getProvisionDeviceSecret() const { return provision_device_secret_; }
    const std::string& getDeviceNamePrefix() const { return device_name_prefix_; }
    std::chrono::seconds getTimeoutSeconds() const { return timeout_seconds_; }
    bool getUseSsl() const { return use_ssl_; }

    // Setters (for testing)
    void setServerUrl(const std::string& url) { server_url_ = url; }
    void setServerPort(int port) { server_port_ = port; }
    void setProvisionDeviceKey(const std::string& key) { provision_device_key_ = key; }
    void setProvisionDeviceSecret(const std::string& secret) { provision_device_secret_ = secret; }
    void setDeviceNamePrefix(const std::string& prefix) { device_name_prefix_ = prefix; }
    void setTimeoutSeconds(std::chrono::seconds timeout) { timeout_seconds_ = timeout; }
    void setUseSsl(bool use_ssl) { use_ssl_ = use_ssl; }

    // Validation
    bool isValid() const;
    std::string getValidationError() const;

    // JSON serialization
    nlohmann::json toJson() const;

private:
    std::string server_url_;
    int server_port_ = 1883;
    std::string provision_device_key_;
    std::string provision_device_secret_;
    std::string device_name_prefix_ = "thermal-camera";
    std::chrono::seconds timeout_seconds_{30};
    bool use_ssl_ = true;

    // Validation helpers
    bool isValidUrl(const std::string& url) const;
    bool isValidPort(int port) const;
    bool isValidDeviceNamePrefix(const std::string& prefix) const;
};

/**
 * @brief Represents device credentials for thermal_config.json
 */
class DeviceCredentials {
public:
    DeviceCredentials() = default;
    
    DeviceCredentials(const std::string& device_id,
                     const std::string& device_name,
                     const std::string& access_token,
                     const std::string& credentials_type);

    // Load from JSON object
    static std::unique_ptr<DeviceCredentials> fromJson(const nlohmann::json& json);

    // Getters
    const std::string& getDeviceId() const { return device_id_; }
    const std::string& getDeviceName() const { return device_name_; }
    const std::string& getAccessToken() const { return access_token_; }
    const std::string& getCredentialsType() const { return credentials_type_; }
    const std::chrono::system_clock::time_point& getProvisionedAt() const { return provisioned_at_; }

    // Setters
    void setDeviceId(const std::string& id) { device_id_ = id; }
    void setDeviceName(const std::string& name) { device_name_ = name; }
    void setAccessToken(const std::string& token) { access_token_ = token; }
    void setCredentialsType(const std::string& type) { credentials_type_ = type; }
    void setProvisionedAt(const std::chrono::system_clock::time_point& time) { provisioned_at_ = time; }

    // Validation
    bool isValid() const;

    // JSON serialization
    nlohmann::json toJson() const;

private:
    std::string device_id_;
    std::string device_name_;
    std::string access_token_;
    std::string credentials_type_ = "ACCESS_TOKEN";
    std::chrono::system_clock::time_point provisioned_at_;

    // Validation helpers
    bool isValidUuid(const std::string& uuid) const;
    bool isValidDeviceName(const std::string& name) const;
    bool isValidAccessToken(const std::string& token) const;
};

/**
 * @brief Configuration manager for thermal_config.json updates
 */
class ThermalConfigManager {
public:
    ThermalConfigManager(const std::string& config_file_path);

    // Load current configuration
    bool loadConfiguration();

    // Update device credentials after successful provisioning
    bool updateDeviceCredentials(const DeviceCredentials& credentials,
                                const std::string& server_url,
                                int server_port,
                                bool use_ssl);

    // Backup and restore operations
    std::string createBackup() const;
    bool restoreFromBackup(const std::string& backup_path);

    // Configuration validation
    bool validateConfiguration() const;

    // Get current configuration as JSON
    nlohmann::json getCurrentConfiguration() const { return current_config_; }

    // Get last error message
    const std::string& getLastError() const { return last_error_; }

private:
    std::string config_file_path_;
    nlohmann::json current_config_;
    std::string last_error_;

    // Helper methods
    bool loadJsonFromFile(const std::string& file_path, nlohmann::json& json);
    bool saveJsonToFile(const std::string& file_path, const nlohmann::json& json);
    std::string generateTimestampedBackupPath() const;
    bool atomicFileUpdate(const std::string& file_path, const nlohmann::json& json);
};

/**
 * @brief Helper functions for configuration operations
 */
namespace config_utils {
    
    /**
     * @brief Check if provision.txt file exists
     */
    bool isProvisioningTriggered(const std::string& base_path = ".");
    
    /**
     * @brief Load provisioning credentials from provision.json
     */
    std::unique_ptr<ProvisioningCredentials> loadProvisioningCredentials(const std::string& base_path = ".");
    
    /**
     * @brief Mark provisioning as completed by renaming provision.txt
     */
    bool markProvisioningCompleted(const std::string& base_path = ".");
    
    /**
     * @brief Validate JSON schema for provisioning credentials
     */
    bool validateProvisioningCredentialsSchema(const nlohmann::json& json);
    
    /**
     * @brief Validate JSON schema for device credentials
     */
    bool validateDeviceCredentialsSchema(const nlohmann::json& json);
    
    /**
     * @brief Generate example provision.json file
     */
    bool generateExampleProvisionFile(const std::string& file_path);
}

} // namespace config