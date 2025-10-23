#pragma once

#include <string>
#include <memory>
#include <sstream>

namespace thermal {

/**
 * @brief Log levels in order of severity
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

/**
 * @brief Simple logging interface
 */
class Logger {
public:
    /**
     * @brief Initialize the logger with configuration
     * @param level Minimum log level to output
     * @param output Output destination ("console", "file", "both")
     * @param log_file File path for file output (ignored if output != "file" or "both")
     */
    static void initialize(LogLevel level, const std::string& output, const std::string& log_file = "");
    
    /**
     * @brief Get the singleton logger instance
     * @return Logger instance
     */
    static Logger& instance();
    
    /**
     * @brief Log a debug message
     * @param message The message to log
     */
    void debug(const std::string& message);
    
    /**
     * @brief Log an info message
     * @param message The message to log
     */
    void info(const std::string& message);
    
    /**
     * @brief Log a warning message
     * @param message The message to log
     */
    void warn(const std::string& message);
    
    /**
     * @brief Log an error message
     * @param message The message to log
     */
    void error(const std::string& message);
    
    /**
     * @brief Check if a log level is enabled
     * @param level The log level to check
     * @return true if the level is enabled
     */
    bool is_enabled(LogLevel level) const;
    
    /**
     * @brief Set the minimum log level
     * @param level New minimum log level
     */
    void set_level(LogLevel level);

private:
    Logger() = default;
    
    void log(LogLevel level, const std::string& message);
    std::string format_message(LogLevel level, const std::string& message) const;
    std::string level_to_string(LogLevel level) const;
    std::string get_timestamp() const;
    
    LogLevel min_level_ = LogLevel::INFO;
    std::string output_mode_ = "console";
    std::string log_file_path_;
    std::shared_ptr<std::ofstream> file_stream_;
};

/**
 * @brief Convenient logging macros
 */
#define LOG_DEBUG(msg) do { \
    if (::thermal::Logger::instance().is_enabled(::thermal::LogLevel::DEBUG)) { \
        std::stringstream ss; ss << msg; \
        ::thermal::Logger::instance().debug(ss.str()); \
    } \
} while(0)

#define LOG_INFO(msg) do { \
    if (::thermal::Logger::instance().is_enabled(::thermal::LogLevel::INFO)) { \
        std::stringstream ss; ss << msg; \
        ::thermal::Logger::instance().info(ss.str()); \
    } \
} while(0)

#define LOG_WARN(msg) do { \
    if (::thermal::Logger::instance().is_enabled(::thermal::LogLevel::WARN)) { \
        std::stringstream ss; ss << msg; \
        ::thermal::Logger::instance().warn(ss.str()); \
    } \
} while(0)

#define LOG_ERROR(msg) do { \
    if (::thermal::Logger::instance().is_enabled(::thermal::LogLevel::ERROR)) { \
        std::stringstream ss; ss << msg; \
        ::thermal::Logger::instance().error(ss.str()); \
    } \
} while(0)

} // namespace thermal