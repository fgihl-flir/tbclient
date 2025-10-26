#pragma once

#include "thingsboard/rpc/rpc_types.h"
#include <memory>
#include <functional>

namespace thermal {

/**
 * @brief Simple timeout manager for RPC commands
 */
class RPCTimeoutManager {
public:
    using TimeoutCallback = std::function<void(const std::string& request_id)>;
    
    /**
     * @brief Check if command has timed out and handle accordingly
     * @param command Command to check
     * @param callback Callback to invoke if timed out
     * @return true if command timed out
     */
    static bool checkAndHandleTimeout(const RPCCommand& command, TimeoutCallback callback);
    
    /**
     * @brief Create timeout error response
     * @param request_id Request ID that timed out
     * @param processing_time_ms Time spent processing
     * @return Timeout error response
     */
    static RPCResponse createTimeoutResponse(const std::string& request_id, int processing_time_ms);
};

} // namespace thermal