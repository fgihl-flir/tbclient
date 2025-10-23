#include "common/logger.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <mutex>

namespace thermal {

// Static member initialization
static std::unique_ptr<Logger> logger_instance = nullptr;
static std::mutex logger_mutex;

void Logger::initialize(LogLevel level, const std::string& output, const std::string& log_file) {
    std::lock_guard<std::mutex> lock(logger_mutex);
    
    if (!logger_instance) {
        logger_instance = std::unique_ptr<Logger>(new Logger());
    }
    
    logger_instance->min_level_ = level;
    logger_instance->output_mode_ = output;
    logger_instance->log_file_path_ = log_file;
    
    // Initialize file stream if needed
    if (output == "file" || output == "both") {
        if (!log_file.empty()) {
            logger_instance->file_stream_ = std::make_shared<std::ofstream>(
                log_file, std::ios::out | std::ios::app);
            
            if (!logger_instance->file_stream_->is_open()) {
                std::cerr << "Failed to open log file: " << log_file << std::endl;
                logger_instance->file_stream_.reset();
            }
        }
    }
}

Logger& Logger::instance() {
    std::lock_guard<std::mutex> lock(logger_mutex);
    
    if (!logger_instance) {
        // Initialize with default settings if not already initialized
        logger_instance = std::unique_ptr<Logger>(new Logger());
    }
    
    return *logger_instance;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

bool Logger::is_enabled(LogLevel level) const {
    return level >= min_level_;
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(logger_mutex);
    min_level_ = level;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!is_enabled(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(logger_mutex);
    
    std::string formatted_message = format_message(level, message);
    
    // Output to console
    if (output_mode_ == "console" || output_mode_ == "both") {
        if (level >= LogLevel::WARN) {
            std::cerr << formatted_message << std::endl;
        } else {
            std::cout << formatted_message << std::endl;
        }
    }
    
    // Output to file
    if ((output_mode_ == "file" || output_mode_ == "both") && file_stream_ && file_stream_->is_open()) {
        *file_stream_ << formatted_message << std::endl;
        file_stream_->flush();  // Ensure immediate write for debugging
    }
}

std::string Logger::format_message(LogLevel level, const std::string& message) const {
    std::stringstream ss;
    ss << "[" << get_timestamp() << "] ";
    ss << "[" << level_to_string(level) << "] ";
    ss << message;
    return ss.str();
}

std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

} // namespace thermal