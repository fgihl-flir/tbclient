#pragma once

#include <stdexcept>
#include <string>
#include <chrono>

namespace thermal {

/**
 * @brief Base exception class for thermal client errors
 */
class ThermalException : public std::exception {
public:
    explicit ThermalException(const std::string& message) 
        : message_(message) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }

protected:
    std::string message_;
};

/**
 * @brief Configuration-related exceptions
 */
class ConfigurationException : public ThermalException {
public:
    explicit ConfigurationException(const std::string& message)
        : ThermalException("Configuration error: " + message) {}
};

/**
 * @brief MQTT connection and communication exceptions
 */
class MQTTException : public ThermalException {
public:
    explicit MQTTException(const std::string& message)
        : ThermalException("MQTT error: " + message) {}
};

/**
 * @brief Telemetry processing exceptions
 */
class TelemetryException : public ThermalException {
public:
    explicit TelemetryException(const std::string& message)
        : ThermalException("Telemetry error: " + message) {}
};

/**
 * @brief Error codes for recoverable errors (not exceptions)
 */
enum class ErrorCode {
    SUCCESS = 0,
    
    // Configuration errors
    CONFIG_FILE_NOT_FOUND = 1001,
    CONFIG_INVALID_JSON = 1002,
    CONFIG_VALIDATION_FAILED = 1003,
    
    // MQTT errors  
    MQTT_CONNECTION_FAILED = 2001,
    MQTT_AUTHENTICATION_FAILED = 2002,
    MQTT_PUBLISH_FAILED = 2003,
    MQTT_DISCONNECTED = 2004,
    
    // Telemetry errors
    TELEMETRY_INVALID_TEMPERATURE = 3001,
    TELEMETRY_INVALID_SPOT_ID = 3002,
    TELEMETRY_TRANSMISSION_FAILED = 3003,
    
    // System errors
    SYSTEM_INITIALIZATION_FAILED = 4001,
    SYSTEM_SHUTDOWN_FAILED = 4002,
    
    // Unknown error
    UNKNOWN_ERROR = 9999
};

/**
 * @brief Result type for operations that can fail
 */
template<typename T>
class Result {
public:
    // Success constructor
    static Result<T> success(T&& value) {
        return Result<T>(std::forward<T>(value));
    }
    
    static Result<T> success(const T& value) {
        return Result<T>(value);
    }
    
    // Error constructor
    static Result<T> error(ErrorCode code, const std::string& message) {
        return Result<T>(code, message);
    }
    
    // Check if result is successful
    bool is_success() const { return error_code_ == ErrorCode::SUCCESS; }
    bool is_error() const { return error_code_ != ErrorCode::SUCCESS; }
    
    // Get value (only valid if is_success() == true)
    const T& value() const {
        if (is_error()) {
            throw std::logic_error("Attempting to get value from error result");
        }
        return value_;
    }
    
    T& value() {
        if (is_error()) {
            throw std::logic_error("Attempting to get value from error result");
        }
        return value_;
    }
    
    // Get error information
    ErrorCode error_code() const { return error_code_; }
    const std::string& error_message() const { return error_message_; }
    
    // Operator overloads for convenience
    explicit operator bool() const { return is_success(); }
    const T& operator*() const { return value(); }
    T& operator*() { return value(); }
    const T* operator->() const { return &value(); }
    T* operator->() { return &value(); }

private:
    // Success constructor
    explicit Result(T&& value) 
        : value_(std::forward<T>(value)), error_code_(ErrorCode::SUCCESS) {}
    explicit Result(const T& value)
        : value_(value), error_code_(ErrorCode::SUCCESS) {}
    
    // Error constructor
    Result(ErrorCode code, const std::string& message)
        : error_code_(code), error_message_(message) {}
    
    T value_{};
    ErrorCode error_code_ = ErrorCode::UNKNOWN_ERROR;
    std::string error_message_;
};

/**
 * @brief Specialization for void operations
 */
template<>
class Result<void> {
public:
    static Result<void> success() {
        return Result<void>(ErrorCode::SUCCESS);
    }
    
    static Result<void> error(ErrorCode code, const std::string& message) {
        return Result<void>(code, message);
    }
    
    bool is_success() const { return error_code_ == ErrorCode::SUCCESS; }
    bool is_error() const { return error_code_ != ErrorCode::SUCCESS; }
    
    ErrorCode error_code() const { return error_code_; }
    const std::string& error_message() const { return error_message_; }
    
    explicit operator bool() const { return is_success(); }

private:
    explicit Result(ErrorCode code, const std::string& message = "")
        : error_code_(code), error_message_(message) {}
    
    ErrorCode error_code_ = ErrorCode::UNKNOWN_ERROR;
    std::string error_message_;
};

/**
 * @brief Utility functions for error handling
 */
class ErrorHandler {
public:
    /**
     * @brief Convert error code to human-readable string
     * @param code Error code
     * @return Error description
     */
    static std::string error_code_to_string(ErrorCode code);
    
    /**
     * @brief Check if an error is recoverable
     * @param code Error code
     * @return true if the operation can be retried
     */
    static bool is_recoverable(ErrorCode code);
    
    /**
     * @brief Create a formatted error message
     * @param code Error code
     * @param context Additional context information
     * @return Formatted error message
     */
    static std::string format_error(ErrorCode code, const std::string& context = "");
};

} // namespace thermal