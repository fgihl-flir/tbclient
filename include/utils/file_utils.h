#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <filesystem>

namespace utils {

/**
 * @brief File backup and management utilities for safe configuration updates
 */
class FileUtils {
public:
    /**
     * @brief Create a timestamped backup of a file
     * @param source_path Path to the file to backup
     * @return Path to the created backup file, empty string on failure
     */
    static std::string createTimestampedBackup(const std::string& source_path);

    /**
     * @brief Create a backup with custom suffix
     * @param source_path Path to the file to backup
     * @param suffix Custom suffix for backup file
     * @return Path to the created backup file, empty string on failure
     */
    static std::string createBackupWithSuffix(const std::string& source_path, const std::string& suffix);

    /**
     * @brief Perform atomic file update (write to temp, then rename)
     * @param file_path Path to the file to update
     * @param content Content to write to the file
     * @return true if update was successful, false otherwise
     */
    static bool atomicFileUpdate(const std::string& file_path, const std::string& content);

    /**
     * @brief Safely rename a file
     * @param old_path Current file path
     * @param new_path New file path
     * @return true if rename was successful, false otherwise
     */
    static bool safeRename(const std::string& old_path, const std::string& new_path);

    /**
     * @brief Check if file exists and is readable
     * @param file_path Path to check
     * @return true if file exists and is readable
     */
    static bool fileExists(const std::string& file_path);

    /**
     * @brief Check if directory exists and is writable
     * @param dir_path Directory path to check
     * @return true if directory exists and is writable
     */
    static bool isDirectoryWritable(const std::string& dir_path);

    /**
     * @brief Get file size in bytes
     * @param file_path Path to the file
     * @return File size in bytes, -1 on error
     */
    static long getFileSize(const std::string& file_path);

    /**
     * @brief Get file modification time
     * @param file_path Path to the file
     * @return File modification time, epoch time on error
     */
    static std::chrono::system_clock::time_point getFileModificationTime(const std::string& file_path);

    /**
     * @brief Read entire file content as string
     * @param file_path Path to the file
     * @return File content as string, empty string on error
     */
    static std::string readFileContent(const std::string& file_path);

    /**
     * @brief Write string content to file
     * @param file_path Path to the file
     * @param content Content to write
     * @return true if write was successful, false otherwise
     */
    static bool writeFileContent(const std::string& file_path, const std::string& content);

    /**
     * @brief List all backup files for a given file
     * @param original_file_path Path to the original file
     * @return Vector of backup file paths, sorted by creation time (newest first)
     */
    static std::vector<std::string> listBackupFiles(const std::string& original_file_path);

    /**
     * @brief Clean up old backup files, keeping only the specified number
     * @param original_file_path Path to the original file
     * @param keep_count Number of backup files to keep (default: 5)
     * @return Number of backup files deleted
     */
    static int cleanupOldBackups(const std::string& original_file_path, int keep_count = 5);

    /**
     * @brief Create directory if it doesn't exist
     * @param dir_path Directory path to create
     * @return true if directory exists or was created successfully
     */
    static bool ensureDirectoryExists(const std::string& dir_path);

    /**
     * @brief Get temporary file path for atomic operations
     * @param original_path Original file path
     * @return Temporary file path
     */
    static std::string getTempFilePath(const std::string& original_path);

    /**
     * @brief Validate file permissions for read/write operations
     * @param file_path Path to check
     * @param require_write Whether write permissions are required
     * @return true if permissions are adequate
     */
    static bool validateFilePermissions(const std::string& file_path, bool require_write = false);

    /**
     * @brief Generate timestamp string for backup files
     * @return Timestamp string in format YYYYMMDD_HHMMSS
     */
    static std::string generateTimestamp();

public:
    /**
     * @brief Copy file with error handling
     * @param source_path Source file path
     * @param dest_path Destination file path
     * @return true if copy was successful
     */
    static bool copyFile(const std::string& source_path, const std::string& dest_path);
};

/**
 * @brief Configuration rollback manager for handling provisioning failures
 */
class ConfigRollbackManager {
public:
    explicit ConfigRollbackManager(const std::string& config_file_path);

    /**
     * @brief Create a rollback checkpoint before making changes
     * @return Checkpoint ID for rollback operations
     */
    std::string createCheckpoint();

    /**
     * @brief Rollback to a specific checkpoint
     * @param checkpoint_id ID returned from createCheckpoint
     * @return true if rollback was successful
     */
    bool rollbackToCheckpoint(const std::string& checkpoint_id);

    /**
     * @brief Commit changes and cleanup rollback data
     * @param checkpoint_id ID returned from createCheckpoint
     */
    void commitCheckpoint(const std::string& checkpoint_id);

    /**
     * @brief Get the latest rollback checkpoint
     * @return Latest checkpoint ID, empty string if none exists
     */
    std::string getLatestCheckpoint() const;

    /**
     * @brief Check if rollback data exists for a checkpoint
     * @param checkpoint_id Checkpoint ID to check
     * @return true if rollback data exists
     */
    bool hasCheckpoint(const std::string& checkpoint_id) const;

private:
    std::string config_file_path_;
    std::string rollback_dir_;

    /**
     * @brief Initialize rollback directory
     */
    void initializeRollbackDirectory();

    /**
     * @brief Generate unique checkpoint ID
     */
    std::string generateCheckpointId() const;

    /**
     * @brief Get checkpoint file path
     */
    std::string getCheckpointFilePath(const std::string& checkpoint_id) const;
};

/**
 * @brief File operation result with error information
 */
struct FileOperationResult {
    bool success;
    std::string error_message;
    std::string result_path;  // For operations that create files

    FileOperationResult(bool success, const std::string& error_message = "", const std::string& result_path = "")
        : success(success), error_message(error_message), result_path(result_path) {}

    operator bool() const { return success; }
};

/**
 * @brief Safe file operations with comprehensive error handling
 */
namespace safe_file_ops {
    
    /**
     * @brief Safely backup a configuration file
     */
    FileOperationResult backupConfigFile(const std::string& config_path);
    
    /**
     * @brief Safely update a configuration file with atomic operations
     */
    FileOperationResult updateConfigFile(const std::string& config_path, const std::string& content);
    
    /**
     * @brief Safely restore a configuration file from backup
     */
    FileOperationResult restoreConfigFile(const std::string& config_path, const std::string& backup_path);
    
    /**
     * @brief Safely rename provision.txt to provision.txt.processed
     */
    FileOperationResult markProvisioningCompleted(const std::string& base_path = ".");
}

} // namespace utils