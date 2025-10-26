#pragma once

#include "thingsboard/rpc/rpc_types.h"
#include <string>
#include <nlohmann/json.hpp>

namespace thermal {

/**
 * @brief RPC message parser and validator for thermal spot commands
 * 
 * Parses incoming RPC JSON messages and validates parameters according
 * to the API contracts defined in contracts/rpc-api.md.
 */
class RPCParser {
public:
    /**
     * @brief Parse RPC command from JSON string
     * @param request_id Request ID from MQTT topic
     * @param json_payload JSON payload from MQTT message
     * @return Parsed RPCCommand structure
     */
    static RPCCommand parseCommand(const std::string& request_id, const std::string& json_payload);
    
    /**
     * @brief Validate RPC command parameters
     * @param command Command to validate
     * @return Empty string if valid, error message if invalid
     */
    static std::string validateCommand(const RPCCommand& command);
    
    /**
     * @brief Parse createSpotMeasurement parameters
     * @param params JSON parameters object
     * @param spotId Output parameter for spot ID
     * @param x Output parameter for X coordinate
     * @param y Output parameter for Y coordinate
     * @return Empty string if valid, error message if invalid
     */
    static std::string parseCreateSpotParams(const nlohmann::json& params, 
                                           std::string& spotId, int& x, int& y);
    
    /**
     * @brief Parse moveSpotMeasurement parameters
     * @param params JSON parameters object
     * @param spotId Output parameter for spot ID
     * @param x Output parameter for new X coordinate
     * @param y Output parameter for new Y coordinate
     * @return Empty string if valid, error message if invalid
     */
    static std::string parseMoveSpotParams(const nlohmann::json& params,
                                         std::string& spotId, int& x, int& y);
    
    /**
     * @brief Parse deleteSpotMeasurement parameters
     * @param params JSON parameters object
     * @param spotId Output parameter for spot ID
     * @return Empty string if valid, error message if invalid
     */
    static std::string parseDeleteSpotParams(const nlohmann::json& params,
                                           std::string& spotId);
    
    /**
     * @brief Validate spot ID format
     * @param spotId Spot ID to validate
     * @return true if spotId is "1", "2", "3", "4", or "5"
     */
    static bool validateSpotId(const std::string& spotId);
    
    /**
     * @brief Validate coordinate values
     * @param x X coordinate
     * @param y Y coordinate
     * @return true if coordinates are within 320x240 bounds
     */
    static bool validateCoordinates(int x, int y);
    
    /**
     * @brief Validate timeout value
     * @param timeout_ms Timeout in milliseconds
     * @return true if timeout is within valid range (1000-30000ms)
     */
    static bool validateTimeout(int timeout_ms);
    
private:
    /**
     * @brief Parse JSON safely with error handling
     * @param json_str JSON string to parse
     * @param result Output JSON object
     * @return true if parsing succeeded
     */
    static bool parseJsonSafely(const std::string& json_str, nlohmann::json& result);
    
    /**
     * @brief Extract required string parameter
     * @param params JSON parameters object
     * @param key Parameter key name
     * @param value Output string value
     * @return true if parameter exists and is a string
     */
    static bool extractStringParam(const nlohmann::json& params, const std::string& key, std::string& value);
    
    /**
     * @brief Extract required integer parameter
     * @param params JSON parameters object
     * @param key Parameter key name
     * @param value Output integer value
     * @return true if parameter exists and is an integer
     */
    static bool extractIntParam(const nlohmann::json& params, const std::string& key, int& value);
};

} // namespace thermal