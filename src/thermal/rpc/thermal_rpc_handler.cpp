#include "thermal/rpc/thermal_rpc_handler.h"
#include "common/logger.h"
#include <set>
#include <cmath>
#include <chrono>
#include <iomanip>

namespace thermal {

ThermalRPCHandler::ThermalRPCHandler(std::shared_ptr<ThermalSpotManager> spot_manager)
    : spot_manager_(spot_manager) {
    if (!spot_manager_) {
        throw std::invalid_argument("ThermalSpotManager cannot be null");
    }
}

void ThermalRPCHandler::setResponseCallback(ResponseCallback callback) {
    response_callback_ = callback;
}

bool ThermalRPCHandler::isSupported(const std::string& method) const {
    static const std::set<std::string> supported_methods = {
        "createSpotMeasurement",
        "moveSpotMeasurement", 
        "deleteSpotMeasurement",
        "listSpotMeasurements",
        "getSpotTemperature"
    };
    
    return supported_methods.find(method) != supported_methods.end();
}

void ThermalRPCHandler::handleRPCCommand(const std::string& request_id, const RPCCommand& command) {
    if (!response_callback_) {
        // Cannot send response without callback - log error
        return;
    }
    
    // Convert method enum to string for comparison
    std::string method_str = RPCCommand::methodToString(command.method);
    
    // Route to appropriate handler based on method
    if (method_str == "createSpotMeasurement") {
        handleCreateSpotMeasurement(request_id, command);
    } else if (method_str == "moveSpotMeasurement") {
        handleMoveSpotMeasurement(request_id, command);
    } else if (method_str == "deleteSpotMeasurement") {
        handleDeleteSpotMeasurement(request_id, command);
    } else if (method_str == "listSpotMeasurements") {
        handleListSpotMeasurements(request_id, command);
    } else if (method_str == "getSpotTemperature") {
        handleGetSpotTemperature(request_id, command);
    } else {
        sendErrorResponse(request_id, RPCErrorCodes::UNKNOWN_METHOD, 
                         "Unsupported thermal RPC method: " + method_str);
    }
}

void ThermalRPCHandler::handleCreateSpotMeasurement(const std::string& request_id, const RPCCommand& command) {
    // Validate required parameters
    if (!validateCreateSpotParams(command)) {
        sendErrorResponse(request_id, RPCErrorCodes::MISSING_PARAMETERS,
                         "Missing required parameters: spotId, x, y");
        return;
    }
    
    // Extract parameters
    std::string spot_id = command.parameters.at("spotId").get<std::string>();
    int x = command.parameters.at("x").get<int>();
    int y = command.parameters.at("y").get<int>();
    
    LOG_INFO("Creating thermal spot: ID=" << spot_id << " at position (" << x << ", " << y << ")");
    
    // Create spot via manager
    bool success = spot_manager_->createSpot(spot_id, x, y);
    
    if (success) {
        // Get temperature reading for the new spot
        float temp = spot_manager_->getSpotTemperature(spot_id);
        
        LOG_INFO("✓ Successfully created spot " << spot_id << " at (" << x << ", " << y << ") - Temperature: " << std::fixed << std::setprecision(2) << temp << "°C");
        
        // Success response with spot details
        nlohmann::json response_data = {
            {"spotId", spot_id},
            {"x", x},
            {"y", y},
            {"status", "created"}
        };
        
        // Include temperature in response if available
        if (!std::isnan(temp)) {
            response_data["temperature"] = temp;
        }
        
        sendSuccessResponse(request_id, response_data);
    } else {
        // Error response - determine likely error based on conditions
        std::string error_code = RPCErrorCodes::INTERNAL_ERROR;
        std::string error_message = "Failed to create spot";
        
        // Check if spot already exists
        if (spot_manager_->spotExists(spot_id)) {
            error_code = RPCErrorCodes::SPOT_ALREADY_EXISTS;
            error_message = "Spot with ID '" + spot_id + "' already exists";
        }
        // Check if coordinates are invalid (basic range check)
        else if (x < 0 || x > 319 || y < 0 || y > 239) {
            error_code = RPCErrorCodes::INVALID_COORDINATES;
            error_message = "Invalid coordinates: x must be 0-319, y must be 0-239";
        }
        // Check if max spots reached
        else if (spot_manager_->getActiveSpotCount() >= 5) {
            error_code = RPCErrorCodes::MAX_SPOTS_REACHED;
            error_message = "Maximum number of spots (5) already created";
        }
        
        LOG_ERROR("✗ Failed to create spot " << spot_id << ": " << error_message);
        sendErrorResponse(request_id, error_code, error_message);
    }
}

void ThermalRPCHandler::handleMoveSpotMeasurement(const std::string& request_id, const RPCCommand& command) {
    // Validate required parameters  
    if (!validateMoveSpotParams(command)) {
        sendErrorResponse(request_id, RPCErrorCodes::MISSING_PARAMETERS,
                         "Missing required parameters: spotId, x, y");
        return;
    }
    
    // Extract parameters
    std::string spot_id = command.parameters.at("spotId").get<std::string>();
    int x = command.parameters.at("x").get<int>();
    int y = command.parameters.at("y").get<int>();
    
    // Check if spot exists first
    if (!spot_manager_->spotExists(spot_id)) {
        sendErrorResponse(request_id, RPCErrorCodes::SPOT_NOT_FOUND, 
                         "Spot with ID '" + spot_id + "' not found");
        return;
    }
    
    // Move spot via manager
    bool success = spot_manager_->moveSpot(spot_id, x, y);
    
    if (success) {
        // Success response with new coordinates
        nlohmann::json response_data = {
            {"spotId", spot_id},
            {"x", x},
            {"y", y},
            {"status", "moved"}
        };
        sendSuccessResponse(request_id, response_data);
    } else {
        // Error response - likely invalid coordinates
        std::string error_code = RPCErrorCodes::INVALID_COORDINATES;
        std::string error_message = "Invalid coordinates: x must be 0-319, y must be 0-239";
        sendErrorResponse(request_id, error_code, error_message);
    }
}

void ThermalRPCHandler::handleDeleteSpotMeasurement(const std::string& request_id, const RPCCommand& command) {
    // Validate required parameters
    if (!validateDeleteSpotParams(command)) {
        sendErrorResponse(request_id, RPCErrorCodes::MISSING_PARAMETERS,
                         "Missing required parameter: spotId");
        return;
    }
    
    // Extract parameters
    std::string spot_id = command.parameters.at("spotId").get<std::string>();
    
    // Check if spot exists first
    if (!spot_manager_->spotExists(spot_id)) {
        sendErrorResponse(request_id, RPCErrorCodes::SPOT_NOT_FOUND, 
                         "Spot with ID '" + spot_id + "' not found");
        return;
    }
    
    // Delete spot via manager
    bool success = spot_manager_->deleteSpot(spot_id);
    
    if (success) {
        // Success response
        nlohmann::json response_data = {
            {"spotId", spot_id},
            {"status", "deleted"}
        };
        sendSuccessResponse(request_id, response_data);
    } else {
        // Error response
        sendErrorResponse(request_id, RPCErrorCodes::INTERNAL_ERROR, 
                         "Failed to delete spot");
    }
}

void ThermalRPCHandler::handleListSpotMeasurements(const std::string& request_id, const RPCCommand& /* command */) {
    LOG_INFO("=== Processing listSpotMeasurements RPC command ===");
    
    // Get all spots from manager
    auto spots = spot_manager_->listSpots();
    
    LOG_INFO("Found " << spots.size() << " active thermal measurement spots:");
    
    // Build response with spot list and log each spot
    nlohmann::json spots_array = nlohmann::json::array();
    for (const auto& spot : spots) {
        nlohmann::json spot_json = {
            {"spotId", std::to_string(spot.id)},
            {"x", spot.x},
            {"y", spot.y}
        };
        
        // Get current temperature for this spot
        float temp = spot_manager_->getSpotTemperature(std::to_string(spot.id));
        if (!std::isnan(temp)) {
            spot_json["temperature"] = temp;
            LOG_INFO("  Spot " << spot.id << ": Position(" << spot.x << ", " << spot.y << ") Temperature: " << std::fixed << std::setprecision(2) << temp << "°C");
        } else {
            LOG_INFO("  Spot " << spot.id << ": Position(" << spot.x << ", " << spot.y << ") Temperature: N/A");
        }
        
        // Include RPC metadata if available
        if (!spot.created_at.empty()) {
            spot_json["createdAt"] = spot.created_at;
        }
        if (!spot.last_reading_at.empty()) {
            spot_json["lastReadingAt"] = spot.last_reading_at;
        }
        
        spots_array.push_back(spot_json);
    }
    
    if (spots.empty()) {
        LOG_INFO("  No active spots found");
    }
    
    nlohmann::json response_data = {
        {"spots", spots_array},
        {"count", spots.size()}
    };
    
    LOG_INFO("Sending listSpotMeasurements response with " << spots.size() << " spots");
    sendSuccessResponse(request_id, response_data);
}

void ThermalRPCHandler::handleGetSpotTemperature(const std::string& request_id, const RPCCommand& command) {
    // Validate required parameters
    if (!validateGetTempParams(command)) {
        sendErrorResponse(request_id, RPCErrorCodes::MISSING_PARAMETERS,
                         "Missing required parameter: spotId");
        return;
    }
    
    // Extract parameters
    std::string spot_id = command.parameters.at("spotId").get<std::string>();
    
    // Check if spot exists first
    if (!spot_manager_->spotExists(spot_id)) {
        sendErrorResponse(request_id, RPCErrorCodes::SPOT_NOT_FOUND, 
                         "Spot with ID '" + spot_id + "' not found");
        return;
    }
    
    // Get temperature reading via manager
    float temperature = spot_manager_->getSpotTemperature(spot_id);
    
    if (!std::isnan(temperature)) {
        // Success response with temperature
        nlohmann::json response_data = {
            {"spotId", spot_id},
            {"temperature", temperature},
            {"timestamp", std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count())}
        };
        sendSuccessResponse(request_id, response_data);
    } else {
        // Error getting temperature
        sendErrorResponse(request_id, RPCErrorCodes::INTERNAL_ERROR, 
                         "Failed to get temperature reading");
    }
}

void ThermalRPCHandler::sendErrorResponse(const std::string& request_id, const std::string& error_code, const std::string& error_message) {
    nlohmann::json response = {
        {"error", {
            {"code", error_code},
            {"message", error_message}
        }}
    };
    
    response_callback_(request_id, response);
}

void ThermalRPCHandler::sendSuccessResponse(const std::string& request_id, const nlohmann::json& data) {
    nlohmann::json response = {
        {"result", data}
    };
    
    response_callback_(request_id, response);
}

bool ThermalRPCHandler::validateCreateSpotParams(const RPCCommand& command) {
    return command.parameters.contains("spotId") && command.parameters["spotId"].is_string() &&
           command.parameters.contains("x") && command.parameters["x"].is_number_integer() &&
           command.parameters.contains("y") && command.parameters["y"].is_number_integer();
}

bool ThermalRPCHandler::validateMoveSpotParams(const RPCCommand& command) {
    return command.parameters.contains("spotId") && command.parameters["spotId"].is_string() &&
           command.parameters.contains("x") && command.parameters["x"].is_number_integer() &&
           command.parameters.contains("y") && command.parameters["y"].is_number_integer();
}

bool ThermalRPCHandler::validateDeleteSpotParams(const RPCCommand& command) {
    return command.parameters.contains("spotId") && command.parameters["spotId"].is_string();
}

bool ThermalRPCHandler::validateGetTempParams(const RPCCommand& command) {
    return command.parameters.contains("spotId") && command.parameters["spotId"].is_string();
}

} // namespace thermal