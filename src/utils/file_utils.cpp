#include "utils/file_utils.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <regex>

namespace utils {

// FileUtils Implementation
std::string FileUtils::createTimestampedBackup(const std::string& source_path) {
    if (!fileExists(source_path)) {
        return "";
    }

    std::string timestamp = generateTimestamp();
    std::string backup_path = source_path + ".backup." + timestamp;

    if (copyFile(source_path, backup_path)) {
        return backup_path;
    }
    return "";
}

std::string FileUtils::createBackupWithSuffix(const std::string& source_path, const std::string& suffix) {
    if (!fileExists(source_path)) {
        return "";
    }

    std::string backup_path = source_path + "." + suffix;
    
    if (copyFile(source_path, backup_path)) {
        return backup_path;
    }
    return "";
}

bool FileUtils::atomicFileUpdate(const std::string& file_path, const std::string& content) {
    std::string temp_path = getTempFilePath(file_path);
    
    // Write to temporary file first
    if (!writeFileContent(temp_path, content)) {
        return false;
    }
    
    // Atomically move temporary file to final location
    if (!safeRename(temp_path, file_path)) {
        // Clean up temporary file if rename failed
        std::filesystem::remove(temp_path);
        return false;
    }
    
    return true;
}

bool FileUtils::safeRename(const std::string& old_path, const std::string& new_path) {
    try {
        std::filesystem::rename(old_path, new_path);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to rename file: " << e.what() << std::endl;
        return false;
    }
}

bool FileUtils::fileExists(const std::string& file_path) {
    try {
        return std::filesystem::exists(file_path) && std::filesystem::is_regular_file(file_path);
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

bool FileUtils::isDirectoryWritable(const std::string& dir_path) {
    try {
        if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
            return false;
        }
        
        // Try to create and delete a temporary file to test write permissions
        std::string test_file = dir_path + "/.write_test_" + generateTimestamp();
        std::ofstream test_stream(test_file);
        if (!test_stream.is_open()) {
            return false;
        }
        test_stream.close();
        
        bool result = std::filesystem::remove(test_file);
        return result;
    } catch (const std::exception&) {
        return false;
    }
}

long FileUtils::getFileSize(const std::string& file_path) {
    try {
        return static_cast<long>(std::filesystem::file_size(file_path));
    } catch (const std::filesystem::filesystem_error&) {
        return -1;
    }
}

std::chrono::system_clock::time_point FileUtils::getFileModificationTime(const std::string& file_path) {
    try {
        auto ftime = std::filesystem::last_write_time(file_path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        return sctp;
    } catch (const std::filesystem::filesystem_error&) {
        return std::chrono::system_clock::time_point{};
    }
}

std::string FileUtils::readFileContent(const std::string& file_path) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return "";
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    } catch (const std::exception&) {
        return "";
    }
}

bool FileUtils::writeFileContent(const std::string& file_path, const std::string& content) {
    try {
        std::ofstream file(file_path);
        if (!file.is_open()) {
            return false;
        }
        
        file << content;
        return file.good();
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<std::string> FileUtils::listBackupFiles(const std::string& original_file_path) {
    std::vector<std::string> backup_files;
    
    try {
        std::filesystem::path original_path(original_file_path);
        std::filesystem::path parent_dir = original_path.parent_path();
        std::string filename = original_path.filename().string();
        
        // Pattern for backup files: filename.backup.YYYYMMDD_HHMMSS
        std::regex backup_pattern(filename + R"(\.backup\.\d{8}_\d{6})");
        
        for (const auto& entry : std::filesystem::directory_iterator(parent_dir)) {
            if (entry.is_regular_file()) {
                std::string entry_filename = entry.path().filename().string();
                if (std::regex_match(entry_filename, backup_pattern)) {
                    backup_files.push_back(entry.path().string());
                }
            }
        }
        
        // Sort by modification time (newest first)
        std::sort(backup_files.begin(), backup_files.end(), 
                 [](const std::string& a, const std::string& b) {
                     return getFileModificationTime(a) > getFileModificationTime(b);
                 });
                 
    } catch (const std::filesystem::filesystem_error&) {
        // Return empty vector on error
    }
    
    return backup_files;
}

int FileUtils::cleanupOldBackups(const std::string& original_file_path, int keep_count) {
    auto backup_files = listBackupFiles(original_file_path);
    
    if (backup_files.size() <= static_cast<size_t>(keep_count)) {
        return 0; // Nothing to clean up
    }
    
    int deleted_count = 0;
    
    // Delete files beyond keep_count (backup_files is sorted newest first)
    for (size_t i = keep_count; i < backup_files.size(); ++i) {
        try {
            if (std::filesystem::remove(backup_files[i])) {
                deleted_count++;
            }
        } catch (const std::filesystem::filesystem_error&) {
            // Continue trying to delete other files even if one fails
        }
    }
    
    return deleted_count;
}

bool FileUtils::ensureDirectoryExists(const std::string& dir_path) {
    try {
        return std::filesystem::create_directories(dir_path) || std::filesystem::exists(dir_path);
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

std::string FileUtils::getTempFilePath(const std::string& original_path) {
    return original_path + ".tmp." + generateTimestamp();
}

bool FileUtils::validateFilePermissions(const std::string& file_path, bool require_write) {
    try {
        if (!std::filesystem::exists(file_path)) {
            return false;
        }
        
        auto perms = std::filesystem::status(file_path).permissions();
        
        // Check read permissions
        if ((perms & std::filesystem::perms::owner_read) == std::filesystem::perms::none &&
            (perms & std::filesystem::perms::group_read) == std::filesystem::perms::none &&
            (perms & std::filesystem::perms::others_read) == std::filesystem::perms::none) {
            return false;
        }
        
        // Check write permissions if required
        if (require_write) {
            if ((perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none &&
                (perms & std::filesystem::perms::group_write) == std::filesystem::perms::none &&
                (perms & std::filesystem::perms::others_write) == std::filesystem::perms::none) {
                return false;
            }
        }
        
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

std::string FileUtils::generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

bool FileUtils::copyFile(const std::string& source_path, const std::string& dest_path) {
    try {
        std::filesystem::copy_file(source_path, dest_path, 
                                  std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Failed to copy file: " << e.what() << std::endl;
        return false;
    }
}

// ConfigRollbackManager Implementation
ConfigRollbackManager::ConfigRollbackManager(const std::string& config_file_path)
    : config_file_path_(config_file_path) {
    
    std::filesystem::path config_path(config_file_path_);
    rollback_dir_ = config_path.parent_path() / ".rollback";
    initializeRollbackDirectory();
}

std::string ConfigRollbackManager::createCheckpoint() {
    if (!FileUtils::fileExists(config_file_path_)) {
        return ""; // No file to checkpoint
    }
    
    std::string checkpoint_id = generateCheckpointId();
    std::string checkpoint_file = getCheckpointFilePath(checkpoint_id);
    
    if (FileUtils::copyFile(config_file_path_, checkpoint_file)) {
        return checkpoint_id;
    }
    return "";
}

bool ConfigRollbackManager::rollbackToCheckpoint(const std::string& checkpoint_id) {
    if (!hasCheckpoint(checkpoint_id)) {
        return false;
    }
    
    std::string checkpoint_file = getCheckpointFilePath(checkpoint_id);
    return FileUtils::copyFile(checkpoint_file, config_file_path_);
}

void ConfigRollbackManager::commitCheckpoint(const std::string& checkpoint_id) {
    if (hasCheckpoint(checkpoint_id)) {
        std::string checkpoint_file = getCheckpointFilePath(checkpoint_id);
        std::filesystem::remove(checkpoint_file);
    }
}

std::string ConfigRollbackManager::getLatestCheckpoint() const {
    try {
        std::vector<std::string> checkpoints;
        
        for (const auto& entry : std::filesystem::directory_iterator(rollback_dir_)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find("checkpoint_") == 0) {
                    checkpoints.push_back(filename.substr(11)); // Remove "checkpoint_" prefix
                }
            }
        }
        
        if (checkpoints.empty()) {
            return "";
        }
        
        // Sort and return latest (timestamps are sortable)
        std::sort(checkpoints.begin(), checkpoints.end());
        return checkpoints.back();
        
    } catch (const std::filesystem::filesystem_error&) {
        return "";
    }
}

bool ConfigRollbackManager::hasCheckpoint(const std::string& checkpoint_id) const {
    if (checkpoint_id.empty()) {
        return false;
    }
    
    std::string checkpoint_file = getCheckpointFilePath(checkpoint_id);
    return FileUtils::fileExists(checkpoint_file);
}

void ConfigRollbackManager::initializeRollbackDirectory() {
    FileUtils::ensureDirectoryExists(rollback_dir_);
}

std::string ConfigRollbackManager::generateCheckpointId() const {
    return FileUtils::generateTimestamp() + "_" + std::to_string(std::random_device{}() % 10000);
}

std::string ConfigRollbackManager::getCheckpointFilePath(const std::string& checkpoint_id) const {
    return rollback_dir_ + "/checkpoint_" + checkpoint_id;
}

// Safe file operations namespace implementation
namespace safe_file_ops {

FileOperationResult backupConfigFile(const std::string& config_path) {
    if (!FileUtils::fileExists(config_path)) {
        return FileOperationResult(false, "Configuration file does not exist: " + config_path);
    }
    
    std::string backup_path = FileUtils::createTimestampedBackup(config_path);
    if (backup_path.empty()) {
        return FileOperationResult(false, "Failed to create backup of configuration file");
    }
    
    return FileOperationResult(true, "", backup_path);
}

FileOperationResult updateConfigFile(const std::string& config_path, const std::string& content) {
    // Create backup first
    auto backup_result = backupConfigFile(config_path);
    if (!backup_result && FileUtils::fileExists(config_path)) {
        return FileOperationResult(false, "Failed to create backup before update: " + backup_result.error_message);
    }
    
    // Perform atomic update
    if (!FileUtils::atomicFileUpdate(config_path, content)) {
        return FileOperationResult(false, "Failed to update configuration file atomically");
    }
    
    return FileOperationResult(true, "", config_path);
}

FileOperationResult restoreConfigFile(const std::string& config_path, const std::string& backup_path) {
    if (!FileUtils::fileExists(backup_path)) {
        return FileOperationResult(false, "Backup file does not exist: " + backup_path);
    }
    
    if (!FileUtils::copyFile(backup_path, config_path)) {
        return FileOperationResult(false, "Failed to restore configuration from backup");
    }
    
    return FileOperationResult(true, "", config_path);
}

FileOperationResult markProvisioningCompleted(const std::string& base_path) {
    std::string provision_file = base_path + "/provision.txt";
    std::string processed_file = base_path + "/provision.txt.processed";
    
    if (!FileUtils::fileExists(provision_file)) {
        return FileOperationResult(false, "Provision file does not exist: " + provision_file);
    }
    
    if (!FileUtils::safeRename(provision_file, processed_file)) {
        return FileOperationResult(false, "Failed to rename provision file to processed");
    }
    
    return FileOperationResult(true, "", processed_file);
}

} // namespace safe_file_ops

} // namespace utils