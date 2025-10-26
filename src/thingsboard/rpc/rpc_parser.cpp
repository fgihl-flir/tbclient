#include "thingsboard/rpc/rpc_parser.h"
#include "common/logger.h"
#include <regex>

namespace thermal {

RPCCommand RPCParser::parseCommand(const std::string& request_id, const std::string& json_payload) {
    RPCCommand command;
    command.requestId = request_id;
    command.receivedAt = std::chrono::system_clock::now();
    command.status = RPCStatus::PENDING;
    
    nlohmann::json json_data;
    if (!parseJsonSafely(json_payload, json_data)) {
        command.status = RPCStatus::ERROR;
        LOG_ERROR("Invalid JSON in RPC command: " << json_payload);
        return command;
    }
    
    // Parse method
    if (!json_data.contains("method") || !json_data["method"].is_string()) {
        command.status = RPCStatus::ERROR;
        LOG_ERROR("Missing or invalid 'method' field in RPC command");
        return command;
    }
    
    std::string method_str = json_data["method"].get<std::string>();
    command.method = RPCCommand::parseMethod(method_str);
    
    if (command.method == RPCMethod::UNKNOWN) {
        command.status = RPCStatus::ERROR;
        LOG_ERROR("Unknown RPC method: " << method_str);
        return command;
    }
    
    // Parse parameters
    if (json_data.contains("params")) {
        command.parameters = json_data["params"];
    } else {
        command.parameters = nlohmann::json::object();
    }
    
    // Parse timeout (optional)
    if (json_data.contains("timeout") && json_data["timeout"].is_number_integer()) {
        command.timeoutMs = json_data["timeout"].get<int>();
    }
    
    LOG_DEBUG("Parsed RPC command: method=" << method_str << ", requestId=" << request_id);
    return command;
}

std::string RPCParser::validateCommand(const RPCCommand& command) {
    // Validate timeout
    if (!validateTimeout(command.timeoutMs)) {
        return "Invalid timeout value: must be between 1000 and 30000 milliseconds";
    }
    
    // Method-specific validation
    switch (command.method) {
        case RPCMethod::CREATE_SPOT_MEASUREMENT: {
            std::string spotId;
            int x, y;
            return parseCreateSpotParams(command.parameters, spotId, x, y);
        }
        
        case RPCMethod::MOVE_SPOT_MEASUREMENT: {
            std::string spotId;
            int x, y;
            return parseMoveSpotParams(command.parameters, spotId, x, y);
        }
        
        case RPCMethod::DELETE_SPOT_MEASUREMENT: {
            std::string spotId;
            return parseDeleteSpotParams(command.parameters, spotId);
        }
        
        case RPCMethod::LIST_SPOT_MEASUREMENTS: {
            // List command requires no parameters
            return "";
        }
        
        case RPCMethod::UNKNOWN:
        default:
            return "Unknown RPC method";
    }
}

std::string RPCParser::parseCreateSpotParams(const nlohmann::json& params, 
                                           std::string& spotId, int& x, int& y) {
    if (!extractStringParam(params, "spotId", spotId)) {
        return "Missing or invalid 'spotId' parameter";
    }
    
    if (!validateSpotId(spotId)) {
        return "Invalid spotId: must be '1', '2', '3', '4', or '5'";
    }
    
    if (!extractIntParam(params, "x", x)) {
        return "Missing or invalid 'x' coordinate parameter";
    }
    
    if (!extractIntParam(params, "y", y)) {
        return "Missing or invalid 'y' coordinate parameter";
    }
    
    if (!validateCoordinates(x, y)) {
        return "Invalid coordinates: x must be 0-319, y must be 0-239";
    }
    
    return ""; // Valid
}

std::string RPCParser::parseMoveSpotParams(const nlohmann::json& params,
                                         std::string& spotId, int& x, int& y) {
    // Same validation as createSpot
    return parseCreateSpotParams(params, spotId, x, y);
}

std::string RPCParser::parseDeleteSpotParams(const nlohmann::json& params,
                                           std::string& spotId) {
    if (!extractStringParam(params, "spotId", spotId)) {
        return "Missing or invalid 'spotId' parameter";
    }
    
    if (!validateSpotId(spotId)) {
        return "Invalid spotId: must be '1', '2', '3', '4', or '5'";
    }
    
    return ""; // Valid
}

bool RPCParser::validateSpotId(const std::string& spotId) {
    return spotId == "1" || spotId == "2" || spotId == "3" || spotId == "4" || spotId == "5";
}

bool RPCParser::validateCoordinates(int x, int y) {
    return x >= 0 && x < 320 && y >= 0 && y < 240;
}

bool RPCParser::validateTimeout(int timeout_ms) {
    return timeout_ms >= 1000 && timeout_ms <= 30000;
}

bool RPCParser::parseJsonSafely(const std::string& json_str, nlohmann::json& result) {
    try {
        result = nlohmann::json::parse(json_str);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("JSON parsing error: " << e.what());
        return false;
    }
}

bool RPCParser::extractStringParam(const nlohmann::json& params, const std::string& key, std::string& value) {
    if (!params.contains(key) || !params[key].is_string()) {
        return false;
    }
    
    value = params[key].get<std::string>();
    return true;
}

bool RPCParser::extractIntParam(const nlohmann::json& params, const std::string& key, int& value) {
    if (!params.contains(key) || !params[key].is_number_integer()) {
        return false;
    }
    
    value = params[key].get<int>();
    return true;
}

} // namespace thermal