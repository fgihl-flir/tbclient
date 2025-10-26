#include <gtest/gtest.h>
#include "thermal/rpc/thermal_rpc_handler.h"
#include "thermal/spot_manager/thermal_spot_manager.h"
#include "thermal/temperature_source/temperature_source_factory.h"
#include "thingsboard/rpc/rpc_types.h"
#include <memory>
#include <filesystem>

namespace thermal {

/**
 * @brief Integration test for createSpotMeasurement RPC flow
 * 
 * Tests the complete end-to-end flow from RPC command reception
 * to spot creation and response generation.
 */
class CreateSpotRPCIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_persistence_file_ = "test_create_spot_rpc.json";
        
        // Clean up any existing test file
        if (std::filesystem::exists(test_persistence_file_)) {
            std::filesystem::remove(test_persistence_file_);
        }
        
        // Create temperature source and spot manager
        auto temp_source = TemperatureSourceFactory::createDefault();
        spot_manager_ = std::make_shared<ThermalSpotManager>(std::move(temp_source), test_persistence_file_);
        
        // Create thermal RPC handler
        rpc_handler_ = std::make_unique<ThermalRPCHandler>(spot_manager_);
        
        // Set up response callback to capture responses
        rpc_handler_->setResponseCallback([this](const std::string& request_id, const nlohmann::json& response) {
            captured_request_id_ = request_id;
            captured_response_ = response;
            response_received_ = true;
        });
        
        // Reset test state
        response_received_ = false;
        captured_request_id_.clear();
        captured_response_.clear();
    }
    
    void TearDown() override {
        rpc_handler_.reset();
        spot_manager_.reset();
        
        // Clean up test file
        if (std::filesystem::exists(test_persistence_file_)) {
            std::filesystem::remove(test_persistence_file_);
        }
    }
    
    std::shared_ptr<ThermalSpotManager> spot_manager_;
    std::unique_ptr<ThermalRPCHandler> rpc_handler_;
    std::string test_persistence_file_;
    
    // Response capture variables
    bool response_received_ = false;
    std::string captured_request_id_;
    nlohmann::json captured_response_;
};

TEST_F(CreateSpotRPCIntegrationTest, CreateSpotSuccess) {
    // Prepare RPC command for createSpotMeasurement
    RPCCommand command;
    command.method = RPCMethod::CREATE_SPOT_MEASUREMENT;
    command.parameters = {
        {"spotId", "1"},
        {"x", 100},
        {"y", 150}
    };
    
    std::string request_id = "test-request-123";
    
    // Execute RPC command
    rpc_handler_->handleRPCCommand(request_id, command);
    
    // Verify response was sent
    ASSERT_TRUE(response_received_);
    EXPECT_EQ(captured_request_id_, request_id);
    
    // Verify success response structure
    ASSERT_TRUE(captured_response_.contains("status"));
    EXPECT_EQ(captured_response_["status"], "success");
    
    ASSERT_TRUE(captured_response_.contains("data"));
    auto data = captured_response_["data"];
    
    EXPECT_EQ(data["spotId"], "1");
    EXPECT_EQ(data["x"], 100);
    EXPECT_EQ(data["y"], 150);
    EXPECT_EQ(data["status"], "created");
    
    // Verify spot was actually created in manager
    auto spots = spot_manager_->listSpots();
    ASSERT_EQ(spots.size(), 1);
    EXPECT_EQ(spots[0].id, "1");
    EXPECT_EQ(spots[0].x, 100);
    EXPECT_EQ(spots[0].y, 150);
    
    // Verify temperature reading is available
    float temp = spot_manager_->getSpotTemperature("1");
    EXPECT_GT(temp, 0.0f);
}

TEST_F(CreateSpotRPCIntegrationTest, CreateSpotDuplicateId) {
    // Create first spot
    ASSERT_TRUE(spot_manager_->createSpot("1", 100, 150));
    
    // Prepare RPC command for duplicate spot
    RPCCommand command;
    command.method = RPCMethod::CREATE_SPOT_MEASUREMENT;
    command.parameters = {
        {"spotId", "1"},
        {"x", 200},
        {"y", 250}
    };
    
    std::string request_id = "test-request-456";
    
    // Execute RPC command
    rpc_handler_->handleRPCCommand(request_id, command);
    
    // Verify error response was sent
    ASSERT_TRUE(response_received_);
    EXPECT_EQ(captured_request_id_, request_id);
    
    // Verify error response structure
    ASSERT_TRUE(captured_response_.contains("status"));
    EXPECT_EQ(captured_response_["status"], "error");
    
    ASSERT_TRUE(captured_response_.contains("error"));
    auto error = captured_response_["error"];
    
    EXPECT_EQ(error["code"], RPCErrorCodes::SPOT_ALREADY_EXISTS);
    EXPECT_TRUE(error["message"].get<std::string>().find("already exists") != std::string::npos);
    
    // Verify only one spot exists (original not overwritten)
    auto spots = spot_manager_->listSpots();
    ASSERT_EQ(spots.size(), 1);
    EXPECT_EQ(spots[0].x, 100);  // Original coordinates
    EXPECT_EQ(spots[0].y, 150);
}

TEST_F(CreateSpotRPCIntegrationTest, CreateSpotInvalidCoordinates) {
    // Test invalid X coordinate
    RPCCommand command;
    command.method = RPCMethod::CREATE_SPOT_MEASUREMENT;
    command.parameters = {
        {"spotId", "1"},
        {"x", -10},  // Invalid X
        {"y", 150}
    };
    
    std::string request_id = "test-request-789";
    
    // Execute RPC command
    rpc_handler_->handleRPCCommand(request_id, command);
    
    // Verify error response
    ASSERT_TRUE(response_received_);
    EXPECT_EQ(captured_response_["status"], "error");
    EXPECT_EQ(captured_response_["error"]["code"], RPCErrorCodes::INVALID_COORDINATES);
    
    // Verify no spot was created
    auto spots = spot_manager_->listSpots();
    EXPECT_EQ(spots.size(), 0);
}

TEST_F(CreateSpotRPCIntegrationTest, CreateSpotMissingParameters) {
    // Test missing required parameter
    RPCCommand command;
    command.method = RPCMethod::CREATE_SPOT_MEASUREMENT;
    command.parameters = {
        {"spotId", "1"},
        // Missing x and y parameters
    };
    
    std::string request_id = "test-request-999";
    
    // Execute RPC command
    rpc_handler_->handleRPCCommand(request_id, command);
    
    // Verify error response
    ASSERT_TRUE(response_received_);
    EXPECT_EQ(captured_response_["status"], "error");
    EXPECT_EQ(captured_response_["error"]["code"], RPCErrorCodes::MISSING_PARAMETERS);
    
    // Verify no spot was created
    auto spots = spot_manager_->listSpots();
    EXPECT_EQ(spots.size(), 0);
}

TEST_F(CreateSpotRPCIntegrationTest, CreateSpotMaxSpotsReached) {
    // Create maximum spots (5)
    for (int i = 1; i <= 5; ++i) {
        ASSERT_TRUE(spot_manager_->createSpot(std::to_string(i), 100 + i * 10, 150 + i * 10));
    }
    
    // Try to create 6th spot via RPC
    RPCCommand command;
    command.method = RPCMethod::CREATE_SPOT_MEASUREMENT;
    command.parameters = {
        {"spotId", "6"},
        {"x", 300},
        {"y", 200}
    };
    
    std::string request_id = "test-request-max";
    
    // Execute RPC command
    rpc_handler_->handleRPCCommand(request_id, command);
    
    // Verify error response
    ASSERT_TRUE(response_received_);
    EXPECT_EQ(captured_response_["status"], "error");
    EXPECT_EQ(captured_response_["error"]["code"], RPCErrorCodes::MAX_SPOTS_REACHED);
    
    // Verify still only 5 spots
    auto spots = spot_manager_->listSpots();
    EXPECT_EQ(spots.size(), 5);
}

TEST_F(CreateSpotRPCIntegrationTest, ResponseTimingPerformance) {
    // Test that response is generated quickly (< 2 seconds as per SC-001)
    auto start_time = std::chrono::steady_clock::now();
    
    RPCCommand command;
    command.method = RPCMethod::CREATE_SPOT_MEASUREMENT;
    command.parameters = {
        {"spotId", "1"},
        {"x", 100},
        {"y", 150}
    };
    
    std::string request_id = "test-request-perf";
    
    // Execute RPC command
    rpc_handler_->handleRPCCommand(request_id, command);
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Verify response was received
    ASSERT_TRUE(response_received_);
    
    // Verify performance requirement (SC-001: < 2 seconds)
    EXPECT_LT(duration.count(), 2000) << "Create spot response took " << duration.count() << "ms (should be < 2000ms)";
    
    // Most operations should be much faster than 2 seconds
    EXPECT_LT(duration.count(), 100) << "Create spot response took " << duration.count() << "ms (expected < 100ms)";
}

} // namespace thermal