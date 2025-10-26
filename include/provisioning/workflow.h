#pragma once

#include "config/provisioning.h"
#include "thingsboard/provisioning.h"
#include "utils/file_utils.h"
#include <string>
#include <memory>

namespace provisioning {

/**
 * @brief Result structure for complete provisioning workflow
 */
struct ProvisioningWorkflowResult {
    bool success;
    std::string error_message;
    std::string device_name;
    std::string access_token;
    int attempts_made;
    std::chrono::milliseconds total_duration;

    ProvisioningWorkflowResult() 
        : success(false), attempts_made(0), total_duration(0) {}
};

/**
 * @brief Statistics for provisioning operations
 */
struct ProvisioningStats {
    int total_attempts;
    int successful_provisions;
    int failed_provisions;
    std::chrono::milliseconds avg_duration;
    std::chrono::system_clock::time_point last_attempt;

    ProvisioningStats() 
        : total_attempts(0), successful_provisions(0), failed_provisions(0), avg_duration(0) {}
};

/**
 * @brief Main provisioning workflow manager
 * 
 * Orchestrates the complete device provisioning process including:
 * - Detection of provision.txt trigger file
 * - Reading provisioning credentials from provision.json
 * - MQTT-based device provisioning with ThingsBoard
 * - Updating thermal_config.json with new device credentials
 * - Clean-up and error handling
 */
class ProvisioningWorkflow {
public:
    /**
     * @brief Construct provisioning workflow with default configuration paths
     */
    ProvisioningWorkflow();

    /**
     * @brief Construct provisioning workflow with custom configuration paths
     * @param base_path Base directory path for configuration files
     * @param broker_host ThingsBoard broker hostname
     * @param broker_port ThingsBoard broker port (default: 1883)
     */
    ProvisioningWorkflow(const std::string& base_path, 
                        const std::string& broker_host, 
                        int broker_port = 1883);

    /**
     * @brief Check if provisioning should be triggered
     * @return true if provision.txt file exists and provisioning should proceed
     */
    bool shouldProvision() const;

    /**
     * @brief Execute the complete provisioning workflow
     * @return ProvisioningWorkflowResult with success status and details
     */
    ProvisioningWorkflowResult executeProvisioning();

    /**
     * @brief Get the last error message from provisioning operations
     * @return Error message string, empty if no error
     */
    std::string getLastError() const;

    /**
     * @brief Get provisioning workflow statistics
     * @return ProvisioningStats with attempt counts and timing
     */
    ProvisioningStats getStats() const;

private:
    std::string base_path_;
    std::string broker_host_;
    int broker_port_;
    std::string last_error_;
    ProvisioningStats stats_;

    // Configuration file paths
    std::string getProvisionTriggerPath() const;
    std::string getProvisionConfigPath() const;
    std::string getThermalConfigPath() const;
    std::string getProcessedTriggerPath() const;

    // Workflow steps
    bool validateProvisioningFiles();
    std::unique_ptr<config::ProvisioningCredentials> loadProvisioningCredentials();
    bool markProvisioningCompleted();
    void cleanupOnFailure();
};

/**
 * @brief Utility functions for provisioning file operations
 */
namespace file_detection {

    /**
     * @brief Check if provision.txt trigger file exists
     * @param base_path Base directory to search (default: current directory)
     * @return true if provision.txt exists and is readable
     */
    bool hasProvisionTrigger(const std::string& base_path = ".");

    /**
     * @brief Check if provision.json configuration file exists and is valid
     * @param base_path Base directory to search (default: current directory)
     * @return true if provision.json exists, is readable, and contains valid JSON
     */
    bool hasValidProvisionConfig(const std::string& base_path = ".");

    /**
     * @brief Check if thermal_config.json exists and is writable
     * @param base_path Base directory to search (default: current directory)
     * @return true if thermal_config.json exists and can be modified
     */
    bool hasThermalConfig(const std::string& base_path = ".");

    /**
     * @brief Validate provision.txt content (should be empty or contain simple trigger text)
     * @param file_path Path to provision.txt file
     * @return true if file content is valid for triggering provisioning
     */
    bool validateProvisionTriggerContent(const std::string& file_path);

    /**
     * @brief Get file modification time for provision files
     * @param file_path Path to file
     * @return File modification timestamp
     */
    std::chrono::system_clock::time_point getProvisionFileTimestamp(const std::string& file_path);

    /**
     * @brief Check if provisioning was already completed (provision.txt.processed exists)
     * @param base_path Base directory to search (default: current directory)
     * @return true if provision.txt.processed exists, indicating previous completion
     */
    bool wasProvisioningCompleted(const std::string& base_path = ".");

    /**
     * @brief Get all provisioning-related files in directory
     * @param base_path Base directory to search
     * @return Vector of provisioning file paths found
     */
    std::vector<std::string> findProvisioningFiles(const std::string& base_path = ".");

    /**
     * @brief Validate directory permissions for provisioning operations
     * @param base_path Base directory to validate
     * @return true if directory has required read/write permissions
     */
    bool validateDirectoryPermissions(const std::string& base_path = ".");

} // namespace file_detection

/**
 * @brief High-level provisioning orchestration functions
 */
namespace orchestration {

    /**
     * @brief Check if provisioning should be triggered and execute if needed
     * @param base_path Base directory for configuration files
     * @param broker_host ThingsBoard broker hostname
     * @param broker_port ThingsBoard broker port
     * @return ProvisioningWorkflowResult with complete operation details
     */
    ProvisioningWorkflowResult checkAndProvision(const std::string& base_path = ".",
                                                const std::string& broker_host = "localhost",
                                                int broker_port = 1883);

    /**
     * @brief Force provisioning execution regardless of trigger file
     * @param base_path Base directory for configuration files
     * @param broker_host ThingsBoard broker hostname
     * @param broker_port ThingsBoard broker port
     * @return ProvisioningWorkflowResult with complete operation details
     */
    ProvisioningWorkflowResult forceProvisioning(const std::string& base_path = ".",
                                                const std::string& broker_host = "localhost",
                                                int broker_port = 1883);

    /**
     * @brief Validate provisioning prerequisites without executing
     * @param base_path Base directory for configuration files
     * @return true if all prerequisites are met for successful provisioning
     */
    bool validateProvisioningPrerequisites(const std::string& base_path = ".");

} // namespace orchestration

} // namespace provisioning