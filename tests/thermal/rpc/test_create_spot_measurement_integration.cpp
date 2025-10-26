#include <gtest/gtest.h>
#include "thingsboard/device.h"
#include "thermal/rpc/thermal_rpc_handler.h"
#include "thermal/spot_manager/thermal_spot_manager.h"
#include "thermal/temperature_source/coordinate_based_source.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <thread>
#include <chrono>

namespace thermal {
namespace test {

/**
 * @brief Integration test for createSpotMeasurement RPC command
 * 
 * Tests the complete flow from RPC message reception through
 * spot creation and response generation.
 */
class CreateSpotMeasurementIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_config_path_ = "/tmp/test_create_spot_integration.json";
        
        // Clean up any existing test file
        std::filesystem::remove(test_config_path_);
        
        // Create thermal spot manager
        spot_manager_ = std::make_shared<ThermalSpotManager>(test_config_path_);
        auto temperature_source = std::make_unique<CoordinateBasedTemperatureSource>();
        spot_manager_->setTemperatureSource(std::move(temperature_source));
        
        // Create thermal RPC handler
        thermal_handler_ = std::make_shared<ThermalRPCHandler>(spot_manager_);
        
        // Set up response capture callback
        thermal_handler_->setResponseCallback(
            [this](const std::string& request_id, const nlohmann::json& response) {
                this->captured_request_id_ = request_id;
                this->captured_response_ = response;
                this->response_received_ = true;
            }
        );
    }
    
    void TearDown() override {
        thermal_handler_.reset();
        spot_manager_.reset();
        std::filesystem::remove(test_config_path_);
    }
    
    std::string test_config_path_;
    std::shared_ptr<ThermalSpotManager> spot_manager_;
    std::shared_ptr<ThermalRPCHandler> thermal_handler_;
    
    // Response capture
    std::string captured_request_id_;
    nlohmann::json captured_response_;
    bool response_received_ = false;
};

TEST_F(CreateSpotMeasurementIntegrationTest, SuccessfulSpotCreation) {
    // Prepare RPC command
    RPCCommand command;
    command.method = "createSpotMeasurement";
    command.params = nlohmann::json{
        {"spotId", "integration-test-spot-1"},
        {"x", 320},
        {"y", 240}
    };
    
    std::string request_id = "test-request-12345";
    
    // Execute RPC command
    thermal_handler_->handleRPCCommand(request_id, command);
    
    // Verify response was generated
    ASSERT_TRUE(response_received_);
    EXPECT_EQ(captured_request_id_, request_id);
    
    // Verify response structure
    ASSERT_TRUE(captured_response_.contains("result"));
    EXPECT_FALSE(captured_response_.contains("error"));
    
    auto result = captured_response_["result"];
    EXPECT_EQ(result["spotId"], "integration-test-spot-1");
    EXPECT_EQ(result["x"], 320);
    EXPECT_EQ(result["y"], 240);
    EXPECT_EQ(result["status"], "created");
    
    // Verify spot was actually created in manager
    auto spots = spot_manager_->listSpots();
    ASSERT_EQ(spots.size(), 1);
    EXPECT_EQ(spots[0].id, "integration-test-spot-1");
    EXPECT_EQ(spots[0].x, 320);
    EXPECT_EQ(spots[0].y, 240);
}

TEST_F(CreateSpotMeasurementIntegrationTest, DuplicateSpotError) {
    // Create first spot
    auto result1 = spot_manager_->createSpot("duplicate-spot", 100, 150);
    ASSERT_TRUE(result1.success);
    
    // Try to create duplicate via RPC
    RPCCommand command;
    command.method = "createSpotMeasurement";
    command.params = nlohmann::json{
        {"spotId", "duplicate-spot"},
        {"x", 200},
        {"y", 250}
    };
    
    std::string request_id = "test-request-duplicate";
    
    // Execute RPC command
    thermal_handler_->handleRPCCommand(request_id, command);
    
    // Verify error response
    ASSERT_TRUE(response_received_);
    EXPECT_EQ(captured_request_id_, request_id);
    
    // Verify error response structure
    ASSERT_TRUE(captured_response_.contains("error"));
    EXPECT_FALSE(captured_response_.contains("result"));
    
    auto error = captured_response_["error"];
    EXPECT_EQ(error["code"], static_cast<int>(RPCErrorCode::INVALID_PARAMS));
    EXPECT_TRUE(error["message"].get<std::string>().find("already exists") != std::string::npos);
    
    // Verify original spot is unchanged
    auto spots = spot_manager_->listSpots();
    ASSERT_EQ(spots.size(), 1);
    EXPECT_EQ(spots[0].x, 100);  // Original coordinates
    EXPECT_EQ(spots[0].y, 150);
}

TEST_F(CreateSpotMeasurementIntegrationTest, InvalidCoordinatesError) {
    // Try to create spot with invalid coordinates
    RPCCommand command;
    command.method = "createSpotMeasurement";
    command.params = nlohmann::json{
        {"spotId", "invalid-coords-spot"},
        {"x", -10},  // Invalid X coordinate
        {"y", 240}
    };
    
    std::string request_id = "test-request-invalid-coords";
    
    // Execute RPC command
    thermal_handler_->handleRPCCommand(request_id, command);
    
    // Verify error response
    ASSERT_TRUE(response_received_);
    EXPECT_EQ(captured_request_id_, request_id);
    
    // Verify error response structure
    ASSERT_TRUE(captured_response_.contains("error"));
    EXPECT_FALSE(captured_response_.contains("result"));
    
    auto error = captured_response_["error"];
    EXPECT_EQ(error["code"], static_cast<int>(RPCErrorCode::INVALID_PARAMS));
    
    // Verify no spot was created
    auto spots = spot_manager_->listSpots();
    EXPECT_EQ(spots.size(), 0);
}

TEST_F(CreateSpotMeasurementIntegrationTest, MissingParametersError) {
    // Try to create spot with missing parameters
    RPCCommand command;
    command.method = "createSpotMeasurement";
    command.params = nlohmann::json{
        {"spotId", "missing-params-spot"}
        // Missing x and y coordinates
    };
    
    std::string request_id = "test-request-missing-params";
    
    // Execute RPC command
    thermal_handler_->handleRPCCommand(request_id, command);
    
    // Verify error response
    ASSERT_TRUE(response_received_);
    EXPECT_EQ(captured_request_id_, request_id);
    
    // Verify error response structure
    ASSERT_TRUE(captured_response_.contains("error"));
    EXPECT_FALSE(captured_response_.contains("result"));
    
    auto error = captured_response_["error"];
    EXPECT_EQ(error["code"], static_cast<int>(RPCErrorCode::INVALID_PARAMS));
    EXPECT_TRUE(error["message"].get<std::string>().find("Missing required parameters") != std::string::npos);
    
    // Verify no spot was created
    auto spots = spot_manager_->listSpots();
    EXPECT_EQ(spots.size(), 0);
}

TEST_F(CreateSpotMeasurementIntegrationTest, MaximumSpotsLimitError) {
    // Create 5 spots (maximum allowed)
    for (int i = 0; i < 5; ++i) {
        auto result = spot_manager_->createSpot("spot-" + std::to_string(i), 100 + i * 10, 150 + i * 10);
        ASSERT_TRUE(result.success) << "Failed to create spot " << i;
    }
    
    // Try to create 6th spot via RPC
    RPCCommand command;
    command.method = "createSpotMeasurement";
    command.params = nlohmann::json{
        {"spotId", "overflow-spot"},
        {"x", 320},
        {"y", 240}
    };
    
    std::string request_id = "test-request-overflow";
    
    // Execute RPC command
    thermal_handler_->handleRPCCommand(request_id, command);
    
    // Verify error response
    ASSERT_TRUE(response_received_);
    EXPECT_EQ(captured_request_id_, request_id);
    
    // Verify error response structure
    ASSERT_TRUE(captured_response_.contains("error"));
    EXPECT_FALSE(captured_response_.contains("result"));
    
    auto error = captured_response_["error"];
    EXPECT_EQ(error["code"], static_cast<int>(RPCErrorCode::INTERNAL_ERROR));
    EXPECT_TRUE(error["message"].get<std::string>().find("maximum") != std::string::npos);
    
    // Verify still only 5 spots
    auto spots = spot_manager_->listSpots();
    EXPECT_EQ(spots.size(), 5);
}

TEST_F(CreateSpotMeasurementIntegrationTest, SpotCreationWithMetadata) {
    // Create spot via RPC
    RPCCommand command;
    command.method = "createSpotMeasurement";
    command.params = nlohmann::json{
        {"spotId", "metadata-test-spot"},
        {"x", 160},
        {"y", 120}
    };
    
    std::string request_id = "test-request-metadata";
    
    // Execute RPC command
    thermal_handler_->handleRPCCommand(request_id, command);
    
    // Verify successful response
    ASSERT_TRUE(response_received_);
    
    // Verify spot was created with RPC metadata
    auto spots = spot_manager_->listSpots();
    ASSERT_EQ(spots.size(), 1);
    
    const auto& spot = spots[0];
    EXPECT_EQ(spot.id, "metadata-test-spot");
    EXPECT_EQ(spot.x, 160);
    EXPECT_EQ(spot.y, 120);
    
    // Verify RPC metadata fields are set
    EXPECT_FALSE(spot.created_at.empty());
    // last_reading_at might be empty initially, that's okay
}

TEST_F(CreateSpotMeasurementIntegrationTest, PersistenceAfterRPCCreation) {
    // Create spot via RPC
    RPCCommand command;
    command.method = "createSpotMeasurement";
    command.params = nlohmann::json{
        {"spotId", "persistence-test-spot"},
        {"x", 400},
        {"y", 300}
    };
    
    std::string request_id = "test-request-persistence";
    
    // Execute RPC command
    thermal_handler_->handleRPCCommand(request_id, command);
    
    // Verify response
    ASSERT_TRUE(response_received_);
    
    // Verify persistence file was created
    EXPECT_TRUE(std::filesystem::exists(test_config_path_));
    
    // Create new manager instance to test loading
    auto new_spot_manager = std::make_shared<ThermalSpotManager>(test_config_path_);
    auto new_temperature_source = std::make_unique<CoordinateBasedTemperatureSource>();
    new_spot_manager->setTemperatureSource(std::move(new_temperature_source));
    
    // Verify spot was loaded
    auto loaded_spots = new_spot_manager->listSpots();
    ASSERT_EQ(loaded_spots.size(), 1);
    
    const auto& loaded_spot = loaded_spots[0];
    EXPECT_EQ(loaded_spot.id, "persistence-test-spot");
    EXPECT_EQ(loaded_spot.x, 400);
    EXPECT_EQ(loaded_spot.y, 300);
}

} // namespace test
} // namespace thermal