#include "config/provisioning.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <sstream>
#include <iomanip>

namespace config {

// ProvisioningCredentials Implementation
std::unique_ptr<ProvisioningCredentials> ProvisioningCredentials::loadFromFile(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open provisioning file: " + file_path);
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Invalid JSON in provisioning file: " + std::string(e.what()));
    }

    return fromJson(j);
}

std::unique_ptr<ProvisioningCredentials> ProvisioningCredentials::fromJson(const nlohmann::json& j) {
    auto creds = std::make_unique<ProvisioningCredentials>();
    
    // Set default values first
    creds->server_url_ = "localhost";
    creds->server_port_ = 1883;
    creds->device_name_prefix_ = "thermal-camera";
    creds->timeout_seconds_ = std::chrono::seconds(30);
    creds->use_ssl_ = false;
    
    // Check if we have the expected nested structure with "provisioning" object
    nlohmann::json provisioning_obj;
    if (j.contains("provisioning") && j["provisioning"].is_object()) {
        provisioning_obj = j["provisioning"];
    } else {
        // Fall back to flat structure for backward compatibility
        provisioning_obj = j;
    }
    
    // Parse JSON fields (supporting both snake_case and camelCase)
    if (provisioning_obj.contains("host") && provisioning_obj["host"].is_string()) {
        creds->server_url_ = provisioning_obj["host"].get<std::string>();
    } else if (provisioning_obj.contains("serverUrl") && provisioning_obj["serverUrl"].is_string()) {
        creds->server_url_ = provisioning_obj["serverUrl"].get<std::string>();
    }
    
    if (provisioning_obj.contains("port") && provisioning_obj["port"].is_number_integer()) {
        creds->server_port_ = provisioning_obj["port"].get<int>();
    } else if (provisioning_obj.contains("serverPort") && provisioning_obj["serverPort"].is_number_integer()) {
        creds->server_port_ = provisioning_obj["serverPort"].get<int>();
    }
    
    // Required fields - device_key/provisionDeviceKey
    if (provisioning_obj.contains("device_key") && provisioning_obj["device_key"].is_string()) {
        creds->provision_device_key_ = provisioning_obj["device_key"].get<std::string>();
    } else if (provisioning_obj.contains("provisionDeviceKey") && provisioning_obj["provisionDeviceKey"].is_string()) {
        creds->provision_device_key_ = provisioning_obj["provisionDeviceKey"].get<std::string>();
    } else {
        throw std::runtime_error("Missing or invalid 'device_key' or 'provisionDeviceKey' field in provisioning file");
    }
    
    // Required fields - device_secret/provisionDeviceSecret
    if (provisioning_obj.contains("device_secret") && provisioning_obj["device_secret"].is_string()) {
        creds->provision_device_secret_ = provisioning_obj["device_secret"].get<std::string>();
    } else if (provisioning_obj.contains("provisionDeviceSecret") && provisioning_obj["provisionDeviceSecret"].is_string()) {
        creds->provision_device_secret_ = provisioning_obj["provisionDeviceSecret"].get<std::string>();
    } else {
        throw std::runtime_error("Missing or invalid 'device_secret' or 'provisionDeviceSecret' field in provisioning file");
    }
    
    if (provisioning_obj.contains("deviceNamePrefix") && provisioning_obj["deviceNamePrefix"].is_string()) {
        creds->device_name_prefix_ = provisioning_obj["deviceNamePrefix"].get<std::string>();
    }
    
    if (provisioning_obj.contains("timeout_seconds") && provisioning_obj["timeout_seconds"].is_number_integer()) {
        creds->timeout_seconds_ = std::chrono::seconds(provisioning_obj["timeout_seconds"].get<int>());
    } else if (provisioning_obj.contains("timeoutSeconds") && provisioning_obj["timeoutSeconds"].is_number_integer()) {
        creds->timeout_seconds_ = std::chrono::seconds(provisioning_obj["timeoutSeconds"].get<int>());
    }
    
    if (provisioning_obj.contains("useSsl") && provisioning_obj["useSsl"].is_boolean()) {
        creds->use_ssl_ = provisioning_obj["useSsl"].get<bool>();
    }
    
    return creds;
}

bool ProvisioningCredentials::isValid() const {
    return !provision_device_key_.empty() && 
           !provision_device_secret_.empty() && 
           !server_url_.empty() && 
           server_port_ > 0 && 
           server_port_ <= 65535;
}

nlohmann::json ProvisioningCredentials::toJson() const {
    nlohmann::json j;
    j["serverUrl"] = server_url_;
    j["serverPort"] = server_port_;
    j["provisionDeviceKey"] = provision_device_key_;
    j["provisionDeviceSecret"] = provision_device_secret_;
    j["deviceNamePrefix"] = device_name_prefix_;
    j["timeoutSeconds"] = timeout_seconds_.count();
    j["useSsl"] = use_ssl_;
    return j;
}

// DeviceCredentials Implementation
DeviceCredentials::DeviceCredentials(const std::string& device_id,
                                   const std::string& device_name,
                                   const std::string& access_token,
                                   const std::string& credentials_type)
    : device_id_(device_id), device_name_(device_name), 
      access_token_(access_token), credentials_type_(credentials_type),
      provisioned_at_(std::chrono::system_clock::now()) {}

std::unique_ptr<DeviceCredentials> DeviceCredentials::fromJson(const nlohmann::json& j) {
    auto creds = std::make_unique<DeviceCredentials>();
    
    // Validate required fields
    if (!j.contains("deviceName") || !j["deviceName"].is_string()) {
        throw std::runtime_error("Missing or invalid 'deviceName' field in device credentials");
    }
    creds->device_name_ = j["deviceName"].get<std::string>();
    
    if (!j.contains("accessToken") || !j["accessToken"].is_string()) {
        throw std::runtime_error("Missing or invalid 'accessToken' field in device credentials");
    }
    creds->access_token_ = j["accessToken"].get<std::string>();

    if (j.contains("deviceId") && j["deviceId"].is_string()) {
        creds->device_id_ = j["deviceId"].get<std::string>();
    }
    
    if (j.contains("credentialsType") && j["credentialsType"].is_string()) {
        creds->credentials_type_ = j["credentialsType"].get<std::string>();
    }
    
    return creds;
}

nlohmann::json DeviceCredentials::toJson() const {
    nlohmann::json j;
    j["deviceName"] = device_name_;
    j["accessToken"] = access_token_;
    if (!device_id_.empty()) {
        j["deviceId"] = device_id_;
    }
    if (!credentials_type_.empty()) {
        j["credentialsType"] = credentials_type_;
    }
    return j;
}

bool DeviceCredentials::isValid() const {
    return !device_name_.empty() && !access_token_.empty();
}

// ThermalConfigManager Implementation
ThermalConfigManager::ThermalConfigManager(const std::string& config_file_path)
    : config_file_path_(config_file_path) {}

bool ThermalConfigManager::loadConfiguration() {
    return loadJsonFromFile(config_file_path_, current_config_);
}

bool ThermalConfigManager::updateDeviceCredentials(const DeviceCredentials& credentials,
                                                  const std::string& server_url,
                                                  int server_port,
                                                  bool use_ssl) {
    try {
        // Update the device configuration
        current_config_["device"]["name"] = credentials.getDeviceName();
        
        // Update MQTT connection settings  
        current_config_["mqtt"]["username"] = credentials.getAccessToken();
        current_config_["mqtt"]["host"] = server_url;
        current_config_["mqtt"]["port"] = server_port;
        current_config_["mqtt"]["ssl"] = use_ssl;
        
        // Update device ID if provided
        if (!credentials.getDeviceId().empty()) {
            current_config_["device"]["id"] = credentials.getDeviceId();
        }
        
        // Save the updated configuration
        return atomicFileUpdate(config_file_path_, current_config_);
        
    } catch (const std::exception& e) {
        last_error_ = "Error updating device credentials: " + std::string(e.what());
        return false;
    }
}

std::string ThermalConfigManager::createBackup() const {
    return generateTimestampedBackupPath();
}

bool ThermalConfigManager::restoreFromBackup(const std::string& backup_path) {
    nlohmann::json backup_config;
    if (!loadJsonFromFile(backup_path, backup_config)) {
        return false;
    }
    
    current_config_ = backup_config;
    return atomicFileUpdate(config_file_path_, current_config_);
}

bool ThermalConfigManager::validateConfiguration() const {
    try {
        return current_config_.contains("device") &&
               current_config_.contains("mqtt") &&
               current_config_["device"].contains("name") &&
               current_config_["mqtt"].contains("username");
    } catch (const std::exception&) {
        return false;
    }
}

bool ThermalConfigManager::loadJsonFromFile(const std::string& file_path, nlohmann::json& json) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            last_error_ = "Cannot open file: " + file_path;
            return false;
        }
        
        file >> json;
        return true;
        
    } catch (const nlohmann::json::exception& e) {
        last_error_ = "Invalid JSON in file: " + std::string(e.what());
        return false;
    } catch (const std::exception& e) {
        last_error_ = "Error reading file: " + std::string(e.what());
        return false;
    }
}

bool ThermalConfigManager::saveJsonToFile(const std::string& file_path, const nlohmann::json& json) {
    try {
        std::ofstream file(file_path);
        if (!file.is_open()) {
            last_error_ = "Cannot write to file: " + file_path;
            return false;
        }
        
        file << json.dump(4);
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = "Error writing file: " + std::string(e.what());
        return false;
    }
}

std::string ThermalConfigManager::generateTimestampedBackupPath() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << config_file_path_ << ".backup." 
       << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

bool ThermalConfigManager::atomicFileUpdate(const std::string& file_path, const nlohmann::json& json) {
    std::string temp_path = file_path + ".tmp";
    
    if (!saveJsonToFile(temp_path, json)) {
        return false;
    }
    
    try {
        std::filesystem::rename(temp_path, file_path);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        last_error_ = "Failed to update file atomically: " + std::string(e.what());
        std::filesystem::remove(temp_path);
        return false;
    }
}

} // namespace config