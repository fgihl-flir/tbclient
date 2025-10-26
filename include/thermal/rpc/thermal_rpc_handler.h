#pragma once

#include "thingsboard/rpc/rpc_types.h"
#include "thermal/spot_manager/thermal_spot_manager.h"
#include <memory>
#include <string>
#include <functional>

namespace thermal {

/**
 * @brief Handler for thermal-related RPC commands from ThingsBoard
 * 
 * Processes RPC commands for thermal spot management operations:
 * - createSpotMeasurement: Create new thermal measurement spot
 * - moveSpotMeasurement: Move existing thermal spot to new coordinates
 * - deleteSpotMeasurement: Remove thermal measurement spot
 * - listSpotMeasurements: Get all active thermal spots
 * - getSpotTemperature: Get current temperature reading for specific spot
 */
class ThermalRPCHandler {
public:
    /**
     * @brief RPC response callback function type
     * @param request_id ThingsBoard RPC request ID for response correlation
     * @param response JSON response payload to send back
     */
    using ResponseCallback = std::function<void(const std::string& request_id, const nlohmann::json& response)>;
    
    /**
     * @brief Constructor
     * @param spot_manager Thermal spot manager instance for spot operations
     */
    explicit ThermalRPCHandler(std::shared_ptr<ThermalSpotManager> spot_manager);
    
    /**
     * @brief Destructor
     */
    ~ThermalRPCHandler() = default;
    
    /**
     * @brief Set response callback for sending RPC responses
     * @param callback Function to call when sending responses back to ThingsBoard
     */
    void setResponseCallback(ResponseCallback callback);
    
    /**
     * @brief Process incoming RPC command
     * @param request_id ThingsBoard RPC request ID
     * @param command Parsed RPC command to process
     * 
     * Validates command, executes appropriate operation, and sends response
     * via the configured response callback.
     */
    void handleRPCCommand(const std::string& request_id, const RPCCommand& command);
    
    /**
     * @brief Check if command is supported by this handler
     * @param method RPC method name
     * @return true if method is supported, false otherwise
     */
    bool isSupported(const std::string& method) const;

private:
    /**
     * @brief Handle createSpotMeasurement RPC command
     * @param request_id RPC request ID
     * @param command RPC command with parameters
     */
    void handleCreateSpotMeasurement(const std::string& request_id, const RPCCommand& command);
    
    /**
     * @brief Handle moveSpotMeasurement RPC command
     * @param request_id RPC request ID
     * @param command RPC command with parameters
     */
    void handleMoveSpotMeasurement(const std::string& request_id, const RPCCommand& command);
    
    /**
     * @brief Handle deleteSpotMeasurement RPC command
     * @param request_id RPC request ID
     * @param command RPC command with parameters
     */
    void handleDeleteSpotMeasurement(const std::string& request_id, const RPCCommand& command);
    
    /**
     * @brief Handle listSpotMeasurements RPC command
     * @param request_id RPC request ID
     * @param command RPC command with parameters
     */
    void handleListSpotMeasurements(const std::string& request_id, const RPCCommand& command);
    
    /**
     * @brief Handle getSpotTemperature RPC command
     * @param request_id RPC request ID
     * @param command RPC command with parameters
     */
    void handleGetSpotTemperature(const std::string& request_id, const RPCCommand& command);
    
    /**
     * @brief Send error response back to ThingsBoard
     * @param request_id RPC request ID
     * @param error_code Error code string
     * @param error_message Error message description
     */
    void sendErrorResponse(const std::string& request_id, const std::string& error_code, const std::string& error_message);
    
    /**
     * @brief Send success response back to ThingsBoard
     * @param request_id RPC request ID
     * @param data Response data payload
     */
    void sendSuccessResponse(const std::string& request_id, const nlohmann::json& data);
    
    /**
     * @brief Validate required parameters for createSpotMeasurement
     * @param command RPC command to validate
     * @return true if valid, false otherwise
     */
    bool validateCreateSpotParams(const RPCCommand& command);
    
    /**
     * @brief Validate required parameters for moveSpotMeasurement
     * @param command RPC command to validate
     * @return true if valid, false otherwise
     */
    bool validateMoveSpotParams(const RPCCommand& command);
    
    /**
     * @brief Validate required parameters for deleteSpotMeasurement
     * @param command RPC command to validate
     * @return true if valid, false otherwise
     */
    bool validateDeleteSpotParams(const RPCCommand& command);
    
    /**
     * @brief Validate required parameters for getSpotTemperature
     * @param command RPC command to validate
     * @return true if valid, false otherwise
     */
    bool validateGetTempParams(const RPCCommand& command);
    
    std::shared_ptr<ThermalSpotManager> spot_manager_;
    ResponseCallback response_callback_;
};

} // namespace thermal