#include "common/error_handler.h"
#include <unordered_map>

namespace thermal {

std::string ErrorHandler::error_code_to_string(ErrorCode code) {
    static const std::unordered_map<ErrorCode, std::string> error_messages = {
        {ErrorCode::SUCCESS, "Success"},
        
        // Configuration errors
        {ErrorCode::CONFIG_FILE_NOT_FOUND, "Configuration file not found"},
        {ErrorCode::CONFIG_INVALID_JSON, "Invalid JSON in configuration file"},
        {ErrorCode::CONFIG_VALIDATION_FAILED, "Configuration validation failed"},
        
        // MQTT errors
        {ErrorCode::MQTT_CONNECTION_FAILED, "MQTT connection failed"},
        {ErrorCode::MQTT_AUTHENTICATION_FAILED, "MQTT authentication failed"},
        {ErrorCode::MQTT_PUBLISH_FAILED, "MQTT publish operation failed"},
        {ErrorCode::MQTT_DISCONNECTED, "MQTT client disconnected"},
        
        // Telemetry errors
        {ErrorCode::TELEMETRY_INVALID_TEMPERATURE, "Invalid temperature reading"},
        {ErrorCode::TELEMETRY_INVALID_SPOT_ID, "Invalid measurement spot ID"},
        {ErrorCode::TELEMETRY_TRANSMISSION_FAILED, "Telemetry transmission failed"},
        
        // System errors
        {ErrorCode::SYSTEM_INITIALIZATION_FAILED, "System initialization failed"},
        {ErrorCode::SYSTEM_SHUTDOWN_FAILED, "System shutdown failed"},
        
        // Unknown error
        {ErrorCode::UNKNOWN_ERROR, "Unknown error"}
    };
    
    auto it = error_messages.find(code);
    if (it != error_messages.end()) {
        return it->second;
    }
    
    return "Unknown error code: " + std::to_string(static_cast<int>(code));
}

bool ErrorHandler::is_recoverable(ErrorCode code) {
    switch (code) {
        // Recoverable errors (can retry)
        case ErrorCode::MQTT_CONNECTION_FAILED:
        case ErrorCode::MQTT_PUBLISH_FAILED:
        case ErrorCode::MQTT_DISCONNECTED:
        case ErrorCode::TELEMETRY_TRANSMISSION_FAILED:
            return true;
            
        // Non-recoverable errors (require user intervention)
        case ErrorCode::CONFIG_FILE_NOT_FOUND:
        case ErrorCode::CONFIG_INVALID_JSON:
        case ErrorCode::CONFIG_VALIDATION_FAILED:
        case ErrorCode::MQTT_AUTHENTICATION_FAILED:
        case ErrorCode::TELEMETRY_INVALID_TEMPERATURE:
        case ErrorCode::TELEMETRY_INVALID_SPOT_ID:
        case ErrorCode::SYSTEM_INITIALIZATION_FAILED:
        case ErrorCode::SYSTEM_SHUTDOWN_FAILED:
            return false;
            
        case ErrorCode::SUCCESS:
            return false;  // No need to retry success
            
        case ErrorCode::UNKNOWN_ERROR:
        default:
            return false;  // Conservative approach for unknown errors
    }
}

std::string ErrorHandler::format_error(ErrorCode code, const std::string& context) {
    std::string base_message = error_code_to_string(code);
    
    if (!context.empty()) {
        return base_message + ": " + context;
    }
    
    return base_message;
}

} // namespace thermal