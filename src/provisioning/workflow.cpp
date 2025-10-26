#include "provisioning/workflow.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>

namespace provisioning {

// ProvisioningWorkflow Implementation
ProvisioningWorkflow::ProvisioningWorkflow() 
    : base_path_("."), broker_host_("localhost"), broker_port_(1883) {}

ProvisioningWorkflow::ProvisioningWorkflow(const std::string& base_path, 
                                         const std::string& broker_host, 
                                         int broker_port)
    : base_path_(base_path), broker_host_(broker_host), broker_port_(broker_port) {
    
    if (broker_host_.empty()) {
        throw std::invalid_argument("Broker host cannot be empty");
    }
    if (broker_port_ <= 0 || broker_port_ > 65535) {
        throw std::invalid_argument("Invalid broker port");
    }
}

bool ProvisioningWorkflow::shouldProvision() const {
    // Provisioning should be triggered if:
    // 1. provision.txt exists (trigger file)
    // 2. provision.json exists and is valid (provisioning credentials)
    // Note: thermal_config.json does NOT need to exist - we'll create/update it during provisioning
    return file_detection::hasProvisionTrigger(base_path_) && 
           file_detection::hasValidProvisionConfig(base_path_);
}

ProvisioningWorkflowResult ProvisioningWorkflow::executeProvisioning() {
    auto start_time = std::chrono::steady_clock::now();
    ProvisioningWorkflowResult result;
    
    stats_.total_attempts++;
    stats_.last_attempt = std::chrono::system_clock::now();
    
    try {
        // Step 1: Validate provisioning files
        if (!validateProvisioningFiles()) {
            result.error_message = "Provisioning validation failed: " + last_error_;
            stats_.failed_provisions++;
            return result;
        }
        
        // Step 2: Load provisioning credentials
        auto creds = loadProvisioningCredentials();
        
        // Step 3: Perform provisioning using the simplified client
        thingsboard::ProvisioningClient client;
        
        bool provisioning_success = client.provision(*creds, 
            [](thingsboard::ProvisioningStatus, const std::string&) {
                // Progress callback - could log progress here
            },
            [](bool, const std::string&) {
                // Completion callback - could handle final result here
            });
        
        result.attempts_made = 1; // Simplified for now
        
        if (!provisioning_success) {
            result.error_message = "Provisioning failed: " + client.getLastError();
            stats_.failed_provisions++;
            cleanupOnFailure();
            return result;
        }
        
        // Step 4: Mark provisioning as completed
        if (!markProvisioningCompleted()) {
            result.error_message = "Failed to mark provisioning as completed: " + last_error_;
            stats_.failed_provisions++;
            cleanupOnFailure();
            return result;
        }
        
        // Step 5: Create thermal_config.json with device credentials from ThingsBoard
        // Get the real credentials from the provisioning client
        std::string device_name = client.getLastDeviceName();
        std::string access_token = client.getLastAccessToken();
        
        if (device_name.empty() || access_token.empty()) {
            result.error_message = "Provisioning succeeded but failed to retrieve device credentials";
            stats_.failed_provisions++;
            cleanupOnFailure();
            return result;
        }
        
        config::DeviceCredentials device_creds(device_name, device_name, access_token, "ACCESS_TOKEN");
        config::ThermalConfigManager config_manager(getThermalConfigPath());
        
        // Create thermal_config.json structure with real ThingsBoard credentials
        nlohmann::json thermal_config = {
            {"thingsboard", {
                {"host", creds->getServerUrl()},
                {"port", creds->getServerPort()},
                {"access_token", access_token},
                {"device_id", device_name},
                {"use_ssl", creds->getUseSsl()},
                {"keep_alive_seconds", 60},
                {"qos_level", 1}
            }},
            {"telemetry", {
                {"interval_seconds", 15},
                {"batch_transmission", false},
                {"retry_attempts", 3},
                {"retry_delay_ms", 1000},
                {"measurement_spots", nlohmann::json::array()}
            }},
            {"logging", {
                {"level", "info"},
                {"output", "console"},
                {"log_file", "thermal-mqtt.log"}
            }}
        };
        
        // Save the thermal configuration
        std::ofstream thermal_file(getThermalConfigPath());
        if (!thermal_file.is_open()) {
            result.error_message = "Failed to create thermal_config.json";
            stats_.failed_provisions++;
            cleanupOnFailure();
            return result;
        }
        thermal_file << thermal_config.dump(4);
        thermal_file.close();
        
        // Success!
        result.success = true;
        result.device_name = device_name;
        result.access_token = access_token;
        stats_.successful_provisions++;
        
    } catch (const std::exception& e) {
        result.error_message = "Provisioning workflow exception: " + std::string(e.what());
        stats_.failed_provisions++;
        cleanupOnFailure();
    }
    
    auto end_time = std::chrono::steady_clock::now();
    result.total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Update average duration
    if (stats_.total_attempts > 0) {
        auto total_ms = stats_.avg_duration.count() * (stats_.total_attempts - 1) + result.total_duration.count();
        stats_.avg_duration = std::chrono::milliseconds(total_ms / stats_.total_attempts);
    }
    
    return result;
}

std::string ProvisioningWorkflow::getLastError() const {
    return last_error_;
}

ProvisioningStats ProvisioningWorkflow::getStats() const {
    return stats_;
}

std::string ProvisioningWorkflow::getProvisionTriggerPath() const {
    return base_path_ + "/provision.txt";
}

std::string ProvisioningWorkflow::getProvisionConfigPath() const {
    return base_path_ + "/provision.json";
}

std::string ProvisioningWorkflow::getThermalConfigPath() const {
    return base_path_ + "/thermal_config.json";
}

std::string ProvisioningWorkflow::getProcessedTriggerPath() const {
    return base_path_ + "/provision.txt.processed";
}

bool ProvisioningWorkflow::validateProvisioningFiles() {
    // Check provision trigger file
    if (!file_detection::hasProvisionTrigger(base_path_)) {
        last_error_ = "Provision trigger file not found: " + getProvisionTriggerPath();
        return false;
    }
    
    // Validate trigger file content
    if (!file_detection::validateProvisionTriggerContent(getProvisionTriggerPath())) {
        last_error_ = "Invalid provision trigger file content";
        return false;
    }
    
    // Check provision configuration file
    if (!file_detection::hasValidProvisionConfig(base_path_)) {
        last_error_ = "Valid provision configuration not found: " + getProvisionConfigPath();
        return false;
    }
    
    // Note: thermal_config.json does NOT need to exist for provisioning
    // We will create/update it during the provisioning process
    
    // Check directory permissions
    if (!file_detection::validateDirectoryPermissions(base_path_)) {
        last_error_ = "Insufficient directory permissions for provisioning operations";
        return false;
    }
    
    return true;
}

std::unique_ptr<config::ProvisioningCredentials> ProvisioningWorkflow::loadProvisioningCredentials() {
    try {
        return config::ProvisioningCredentials::loadFromFile(getProvisionConfigPath());
    } catch (const std::exception& e) {
        last_error_ = "Failed to load provisioning credentials: " + std::string(e.what());
        throw;
    }
}

bool ProvisioningWorkflow::markProvisioningCompleted() {
    auto result = utils::safe_file_ops::markProvisioningCompleted(base_path_);
    if (!result) {
        last_error_ = result.error_message;
        return false;
    }
    return true;
}

void ProvisioningWorkflow::cleanupOnFailure() {
    // Implementation for cleanup operations on failure
    // For now, just log the cleanup attempt
    std::cerr << "Performing cleanup after provisioning failure" << std::endl;
}

// File detection namespace implementation
namespace file_detection {

bool hasProvisionTrigger(const std::string& base_path) {
    std::string trigger_path = base_path + "/provision.txt";
    return utils::FileUtils::fileExists(trigger_path);
}

bool hasValidProvisionConfig(const std::string& base_path) {
    std::string config_path = base_path + "/provision.json";
    
    if (!utils::FileUtils::fileExists(config_path)) {
        return false;
    }
    
    try {
        // Try to load and validate the configuration
        auto creds = config::ProvisioningCredentials::loadFromFile(config_path);
        return creds->isValid();
    } catch (const std::exception&) {
        return false;
    }
}

bool hasThermalConfig(const std::string& base_path) {
    std::string config_path = base_path + "/thermal_config.json";
    return utils::FileUtils::fileExists(config_path) && 
           utils::FileUtils::validateFilePermissions(config_path, true);
}

bool validateProvisionTriggerContent(const std::string& file_path) {
    if (!utils::FileUtils::fileExists(file_path)) {
        return false;
    }
    
    // Read file content
    std::string content = utils::FileUtils::readFileContent(file_path);
    
    // Trigger file can be empty or contain simple text
    // Validate that it's not binary or excessively large
    if (content.size() > 1024) { // Max 1KB for trigger file
        return false;
    }
    
    // Check for printable characters only
    for (char c : content) {
        if (c != '\n' && c != '\r' && c != '\t' && (c < 32 || c > 126)) {
            return false;
        }
    }
    
    return true;
}

std::chrono::system_clock::time_point getProvisionFileTimestamp(const std::string& file_path) {
    return utils::FileUtils::getFileModificationTime(file_path);
}

bool wasProvisioningCompleted(const std::string& base_path) {
    std::string processed_path = base_path + "/provision.txt.processed";
    return utils::FileUtils::fileExists(processed_path);
}

std::vector<std::string> findProvisioningFiles(const std::string& base_path) {
    std::vector<std::string> files;
    
    std::vector<std::string> candidate_files = {
        base_path + "/provision.txt",
        base_path + "/provision.json",
        base_path + "/thermal_config.json",
        base_path + "/provision.txt.processed"
    };
    
    for (const auto& file : candidate_files) {
        if (utils::FileUtils::fileExists(file)) {
            files.push_back(file);
        }
    }
    
    return files;
}

bool validateDirectoryPermissions(const std::string& base_path) {
    return utils::FileUtils::isDirectoryWritable(base_path);
}

} // namespace file_detection

// Orchestration namespace implementation
namespace orchestration {

ProvisioningWorkflowResult checkAndProvision(const std::string& base_path,
                                           const std::string& broker_host,
                                           int broker_port) {
    ProvisioningWorkflow workflow(base_path, broker_host, broker_port);
    
    if (!workflow.shouldProvision()) {
        ProvisioningWorkflowResult result;
        result.success = false;
        result.error_message = "Provisioning not triggered - missing required files or conditions";
        return result;
    }
    
    return workflow.executeProvisioning();
}

ProvisioningWorkflowResult forceProvisioning(const std::string& base_path,
                                           const std::string& broker_host,
                                           int broker_port) {
    ProvisioningWorkflow workflow(base_path, broker_host, broker_port);
    return workflow.executeProvisioning();
}

bool validateProvisioningPrerequisites(const std::string& base_path) {
    return file_detection::hasValidProvisionConfig(base_path) &&
           file_detection::hasThermalConfig(base_path) &&
           file_detection::validateDirectoryPermissions(base_path);
}

} // namespace orchestration

} // namespace provisioning