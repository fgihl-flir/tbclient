#include "thingsboard/rpc/rpc_types.h"
#include <map>

namespace thermal {

RPCMethod RPCCommand::parseMethod(const std::string& method_str) {
    static const std::map<std::string, RPCMethod> method_map = {
        {"createSpotMeasurement", RPCMethod::CREATE_SPOT_MEASUREMENT},
        {"moveSpotMeasurement", RPCMethod::MOVE_SPOT_MEASUREMENT},
        {"deleteSpotMeasurement", RPCMethod::DELETE_SPOT_MEASUREMENT},
        {"listSpotMeasurements", RPCMethod::LIST_SPOT_MEASUREMENTS},
        {"getSpotTemperature", RPCMethod::GET_SPOT_TEMPERATURE}
    };
    
    auto it = method_map.find(method_str);
    return (it != method_map.end()) ? it->second : RPCMethod::UNKNOWN;
}

std::string RPCCommand::methodToString(RPCMethod method) {
    switch (method) {
        case RPCMethod::CREATE_SPOT_MEASUREMENT:
            return "createSpotMeasurement";
        case RPCMethod::MOVE_SPOT_MEASUREMENT:
            return "moveSpotMeasurement";
        case RPCMethod::DELETE_SPOT_MEASUREMENT:
            return "deleteSpotMeasurement";
        case RPCMethod::LIST_SPOT_MEASUREMENTS:
            return "listSpotMeasurements";
        case RPCMethod::GET_SPOT_TEMPERATURE:
            return "getSpotTemperature";
        case RPCMethod::UNKNOWN:
        default:
            return "unknown";
    }
}

bool RPCCommand::isTimedOut() const {
    if (status == RPCStatus::COMPLETED || status == RPCStatus::ERROR || status == RPCStatus::TIMEOUT) {
        return false; // Already completed
    }
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - receivedAt).count();
    
    return elapsed > timeoutMs;
}

int RPCCommand::getProcessingDurationMs() const {
    auto now = std::chrono::system_clock::now();
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - receivedAt).count());
}

RPCResponse RPCResponse::createSuccess(const std::string& request_id,
                                     const nlohmann::json& response_data,
                                     int processing_time_ms) {
    RPCResponse response;
    response.requestId = request_id;
    response.success = true;
    response.data = response_data;
    response.responseTimeMs = processing_time_ms;
    response.sentAt = std::chrono::system_clock::now();
    
    return response;
}

RPCResponse RPCResponse::createError(const std::string& request_id,
                                   const std::string& error_code,
                                   const std::string& error_message,
                                   int processing_time_ms) {
    RPCResponse response;
    response.requestId = request_id;
    response.success = false;
    response.errorCode = error_code;
    response.errorMessage = error_message;
    response.responseTimeMs = processing_time_ms;
    response.sentAt = std::chrono::system_clock::now();
    
    return response;
}

nlohmann::json RPCResponse::toJson() const {
    nlohmann::json json_response;
    
    if (success) {
        json_response["result"] = "success";
        json_response["data"] = data;
    } else {
        json_response["result"] = "error";
        json_response["error"] = {
            {"code", errorCode},
            {"message", errorMessage}
        };
    }
    
    return json_response;
}

std::string RPCResponse::toJsonString() const {
    return toJson().dump();
}

} // namespace thermal