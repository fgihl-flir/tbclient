#include "thingsboard/rpc/rpc_timeout_manager.h"

namespace thermal {

bool RPCTimeoutManager::checkAndHandleTimeout(const RPCCommand& command, TimeoutCallback callback) {
    if (command.isTimedOut()) {
        if (callback) {
            callback(command.requestId);
        }
        return true;
    }
    return false;
}

RPCResponse RPCTimeoutManager::createTimeoutResponse(const std::string& request_id, int processing_time_ms) {
    return RPCResponse::createError(request_id, RPCErrorCodes::TIMEOUT, 
                                  "RPC command exceeded timeout limit", processing_time_ms);
}

} // namespace thermal