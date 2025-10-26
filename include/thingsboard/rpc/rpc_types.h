#pragma once

#include <string>
#include <chrono>
#include <nlohmann/json.hpp>

namespace thermal {

/**
 * @brief RPC command processing status
 */
enum class RPCStatus {
    PENDING,     // Command received, not yet processed
    PROCESSING,  // Command currently being processed
    COMPLETED,   // Command completed successfully
    ERROR,       // Command failed with error
    TIMEOUT      // Command exceeded timeout limit
};

/**
 * @brief RPC command method types
 */
enum class RPCMethod {
    CREATE_SPOT_MEASUREMENT,
    MOVE_SPOT_MEASUREMENT,
    DELETE_SPOT_MEASUREMENT,
    LIST_SPOT_MEASUREMENTS,
    GET_SPOT_TEMPERATURE,
    UNKNOWN
};

/**
 * @brief RPC command structure for thermal spot operations
 */
struct RPCCommand {
    std::string requestId;                    // Unique request identifier from MQTT topic
    RPCMethod method = RPCMethod::UNKNOWN;    // RPC method to execute
    nlohmann::json parameters;                // Method-specific parameters
    std::chrono::time_point<std::chrono::system_clock> receivedAt;  // When command was received
    std::chrono::time_point<std::chrono::system_clock> processedAt; // When processing completed
    int timeoutMs = 5000;                     // Command timeout in milliseconds
    RPCStatus status = RPCStatus::PENDING;    // Current processing status
    
    /**
     * @brief Parse RPC method from string
     * @param method_str Method name string
     * @return Corresponding RPCMethod enum
     */
    static RPCMethod parseMethod(const std::string& method_str);
    
    /**
     * @brief Convert RPCMethod to string
     * @param method Method enum value
     * @return Method name string
     */
    static std::string methodToString(RPCMethod method);
    
    /**
     * @brief Check if command has exceeded timeout
     * @return true if command should be considered timed out
     */
    bool isTimedOut() const;
    
    /**
     * @brief Get processing duration in milliseconds
     * @return Duration since command was received
     */
    int getProcessingDurationMs() const;
};

/**
 * @brief RPC response structure for thermal spot operations
 */
struct RPCResponse {
    std::string requestId;                    // Matches originating command request ID
    bool success = false;                     // High-level result status
    nlohmann::json data;                      // Success response data (method-specific)
    std::string errorCode;                    // Error code for failed responses
    std::string errorMessage;                 // Human-readable error description
    int responseTimeMs = 0;                   // Processing time in milliseconds
    std::chrono::time_point<std::chrono::system_clock> sentAt;  // When response was sent
    
    /**
     * @brief Create success response
     * @param request_id Request ID to respond to
     * @param response_data Success data payload
     * @param processing_time_ms Processing duration
     * @return Success response structure
     */
    static RPCResponse createSuccess(const std::string& request_id,
                                   const nlohmann::json& response_data,
                                   int processing_time_ms = 0);
    
    /**
     * @brief Create error response
     * @param request_id Request ID to respond to
     * @param error_code Error code identifier
     * @param error_message Human-readable error description
     * @param processing_time_ms Processing duration
     * @return Error response structure
     */
    static RPCResponse createError(const std::string& request_id,
                                 const std::string& error_code,
                                 const std::string& error_message,
                                 int processing_time_ms = 0);
    
    /**
     * @brief Convert response to JSON for MQTT publishing
     * @return JSON representation following ThingsBoard RPC response format
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief Convert response to JSON string
     * @return JSON string for MQTT payload
     */
    std::string toJsonString() const;
};

/**
 * @brief Common RPC error codes for thermal spot operations
 */
namespace RPCErrorCodes {
    constexpr const char* SPOT_ALREADY_EXISTS = "SPOT_ALREADY_EXISTS";
    constexpr const char* SPOT_NOT_FOUND = "SPOT_NOT_FOUND";
    constexpr const char* INVALID_COORDINATES = "INVALID_COORDINATES";
    constexpr const char* MAX_SPOTS_REACHED = "MAX_SPOTS_REACHED";
    constexpr const char* UNKNOWN_METHOD = "UNKNOWN_METHOD";
    constexpr const char* INVALID_JSON = "INVALID_JSON";
    constexpr const char* MISSING_PARAMETERS = "MISSING_PARAMETERS";
    constexpr const char* CAMERA_BUSY = "CAMERA_BUSY";
    constexpr const char* INTERNAL_ERROR = "INTERNAL_ERROR";
    constexpr const char* TIMEOUT = "TIMEOUT";
    constexpr const char* INVALID_SPOT_ID = "INVALID_SPOT_ID";
}

} // namespace thermal