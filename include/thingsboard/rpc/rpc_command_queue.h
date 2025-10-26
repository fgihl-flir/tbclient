#pragma once

#include "thingsboard/rpc/rpc_types.h"
#include <queue>
#include <memory>

namespace thermal {

/**
 * @brief Simple sequential RPC command queue
 */
class RPCCommandQueue {
private:
    std::queue<std::unique_ptr<RPCCommand>> queue_;
    bool processing_ = false;
    
public:
    /**
     * @brief Add command to queue
     * @param command Command to queue
     */
    void enqueue(std::unique_ptr<RPCCommand> command);
    
    /**
     * @brief Get next command for processing
     * @return Next command or nullptr if queue empty
     */
    std::unique_ptr<RPCCommand> dequeue();
    
    /**
     * @brief Check if queue is empty
     * @return true if no commands queued
     */
    bool isEmpty() const;
    
    /**
     * @brief Set processing state
     * @param processing Whether a command is currently being processed
     */
    void setProcessing(bool processing);
    
    /**
     * @brief Check if a command is currently being processed
     * @return true if processing
     */
    bool isProcessing() const;
    
    /**
     * @brief Get queue size
     * @return Number of queued commands
     */
    size_t size() const;
};

} // namespace thermal