#include "thingsboard/rpc/rpc_command_queue.h"

namespace thermal {

void RPCCommandQueue::enqueue(std::unique_ptr<RPCCommand> command) {
    queue_.push(std::move(command));
}

std::unique_ptr<RPCCommand> RPCCommandQueue::dequeue() {
    if (queue_.empty()) {
        return nullptr;
    }
    
    auto command = std::move(queue_.front());
    queue_.pop();
    return command;
}

bool RPCCommandQueue::isEmpty() const {
    return queue_.empty();
}

void RPCCommandQueue::setProcessing(bool processing) {
    processing_ = processing;
}

bool RPCCommandQueue::isProcessing() const {
    return processing_;
}

size_t RPCCommandQueue::size() const {
    return queue_.size();
}

} // namespace thermal