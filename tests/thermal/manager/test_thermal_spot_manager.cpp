#include <gtest/gtest.h>
#include "thermal/spot_manager/thermal_spot_manager.h"
#include "thermal/temperature_source/coordinate_based_source.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

namespace thermal {
namespace test {

class ThermalSpotManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_config_path_ = "/tmp/test_thermal_spots.json";
        // Clean up any existing test file
        std::filesystem::remove(test_config_path_);
        
        // Create temperature source
        temperature_source_ = std::make_unique<CoordinateBasedTemperatureSource>();
        
        // Create manager with test config path
        manager_ = std::make_unique<ThermalSpotManager>(test_config_path_);
        manager_->setTemperatureSource(std::move(temperature_source_));
    }
    
    void TearDown() override {
        manager_.reset();
        std::filesystem::remove(test_config_path_);
    }
    
    std::string test_config_path_;
    std::unique_ptr<ThermalSpotManager> manager_;
    std::unique_ptr<TemperatureDataSource> temperature_source_;
};

TEST_F(ThermalSpotManagerTest, CreateSpotSuccess) {
    // Test creating a valid spot
    bool result = manager_->createSpot("test-spot-1", 100, 150);
    
    ASSERT_TRUE(result);
    
    // Verify spot exists in list
    auto spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 1);
    EXPECT_EQ(spots[0].id, "test-spot-1");
    EXPECT_EQ(spots[0].x, 100);
    EXPECT_EQ(spots[0].y, 150);
}

TEST_F(ThermalSpotManagerTest, CreateSpotDuplicateId) {
    // Create first spot
    bool result1 = manager_->createSpot("test-spot", 100, 150);
    ASSERT_TRUE(result1);
    
    // Try to create spot with same ID - should fail
    bool result2 = manager_->createSpot("test-spot", 200, 250);
    EXPECT_FALSE(result2);
}

TEST_F(ThermalSpotManagerTest, CreateSpotInvalidCoordinates) {
    // Test invalid X coordinate
    bool result1 = manager_->createSpot("test-spot-1", -10, 150);
    EXPECT_FALSE(result1);
    
    // Test invalid Y coordinate
    bool result2 = manager_->createSpot("test-spot-2", 100, 1500);
    EXPECT_FALSE(result2);
}

TEST_F(ThermalSpotManagerTest, CreateSpotMaximumLimit) {
    // Create 5 spots (maximum allowed)
    for (int i = 0; i < 5; ++i) {
        bool result = manager_->createSpot("spot-" + std::to_string(i), 100 + i * 10, 150 + i * 10);
        ASSERT_TRUE(result) << "Failed to create spot " << i;
    }
    
    // Try to create 6th spot - should fail
    bool result = manager_->createSpot("spot-6", 200, 250);
    EXPECT_FALSE(result);
}
    EXPECT_TRUE(result.error_message.find("maximum") != std::string::npos);
}

TEST_F(ThermalSpotManagerTest, MoveSpotSuccess) {
    // Create a spot first
    auto create_result = manager_->createSpot("test-spot", 100, 150);
    ASSERT_TRUE(create_result.success);
    
    // Move the spot
    auto move_result = manager_->moveSpot("test-spot", 200, 250);
    EXPECT_TRUE(move_result.success);
    EXPECT_EQ(move_result.error_code, RPCErrorCode::SUCCESS);
    
    // Verify new coordinates
    auto spots = manager_->listSpots();
    ASSERT_EQ(spots.size(), 1);
    EXPECT_EQ(spots[0].x, 200);
    EXPECT_EQ(spots[0].y, 250);
}

TEST_F(ThermalSpotManagerTest, MoveSpotNotFound) {
    // Try to move non-existent spot
    auto result = manager_->moveSpot("non-existent", 200, 250);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, RPCErrorCode::INVALID_PARAMS);
    EXPECT_TRUE(result.error_message.find("not found") != std::string::npos);
}

TEST_F(ThermalSpotManagerTest, MoveSpotInvalidCoordinates) {
    // Create a spot first
    auto create_result = manager_->createSpot("test-spot", 100, 150);
    ASSERT_TRUE(create_result.success);
    
    // Try to move to invalid coordinates
    auto result = manager_->moveSpot("test-spot", -10, 150);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, RPCErrorCode::INVALID_PARAMS);
}

TEST_F(ThermalSpotManagerTest, DeleteSpotSuccess) {
    // Create a spot first
    auto create_result = manager_->createSpot("test-spot", 100, 150);
    ASSERT_TRUE(create_result.success);
    
    // Delete the spot
    auto delete_result = manager_->deleteSpot("test-spot");
    EXPECT_TRUE(delete_result.success);
    EXPECT_EQ(delete_result.error_code, RPCErrorCode::SUCCESS);
    
    // Verify spot is gone
    auto spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 0);
}

TEST_F(ThermalSpotManagerTest, DeleteSpotNotFound) {
    // Try to delete non-existent spot
    auto result = manager_->deleteSpot("non-existent");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, RPCErrorCode::INVALID_PARAMS);
    EXPECT_TRUE(result.error_message.find("not found") != std::string::npos);
}

TEST_F(ThermalSpotManagerTest, ListSpotsEmpty) {
    auto spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 0);
}

TEST_F(ThermalSpotManagerTest, ListSpotsMultiple) {
    // Create multiple spots
    std::vector<std::string> spot_ids = {"spot-1", "spot-2", "spot-3"};
    for (size_t i = 0; i < spot_ids.size(); ++i) {
        auto result = manager_->createSpot(spot_ids[i], 100 + i * 10, 150 + i * 10);
        ASSERT_TRUE(result.success);
    }
    
    // List spots
    auto spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), spot_ids.size());
    
    // Verify all spot IDs are present
    for (const auto& spot : spots) {
        EXPECT_TRUE(std::find(spot_ids.begin(), spot_ids.end(), spot.id) != spot_ids.end());
    }
}

TEST_F(ThermalSpotManagerTest, GetTemperatureReading) {
    // Create a spot
    auto create_result = manager_->createSpot("test-spot", 100, 150);
    ASSERT_TRUE(create_result.success);
    
    // Get temperature reading
    auto temp_result = manager_->getTemperatureReading("test-spot");
    EXPECT_TRUE(temp_result.success);
    EXPECT_EQ(temp_result.error_code, RPCErrorCode::SUCCESS);
    EXPECT_GT(temp_result.temperature, 0.0);  // Should be positive temperature
    EXPECT_LT(temp_result.temperature, 100.0);  // Should be reasonable range
}

TEST_F(ThermalSpotManagerTest, GetTemperatureReadingNotFound) {
    // Try to get temperature for non-existent spot
    auto result = manager_->getTemperatureReading("non-existent");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code, RPCErrorCode::INVALID_PARAMS);
    EXPECT_TRUE(result.error_message.find("not found") != std::string::npos);
}

TEST_F(ThermalSpotManagerTest, PersistenceIntegration) {
    // Create spots
    auto result1 = manager_->createSpot("spot-1", 100, 150);
    auto result2 = manager_->createSpot("spot-2", 200, 250);
    ASSERT_TRUE(result1.success);
    ASSERT_TRUE(result2.success);
    
    // Verify persistence file was created
    EXPECT_TRUE(std::filesystem::exists(test_config_path_));
    
    // Create new manager instance to test loading
    auto new_manager = std::make_unique<ThermalSpotManager>(test_config_path_);
    new_manager->setTemperatureSource(std::make_unique<CoordinateBasedTemperatureSource>());
    
    // Verify spots were loaded
    auto loaded_spots = new_manager->listSpots();
    EXPECT_EQ(loaded_spots.size(), 2);
    
    // Find and verify each spot
    auto spot1_it = std::find_if(loaded_spots.begin(), loaded_spots.end(),
        [](const MeasurementSpot& spot) { return spot.id == "spot-1"; });
    auto spot2_it = std::find_if(loaded_spots.begin(), loaded_spots.end(),
        [](const MeasurementSpot& spot) { return spot.id == "spot-2"; });
    
    ASSERT_NE(spot1_it, loaded_spots.end());
    ASSERT_NE(spot2_it, loaded_spots.end());
    
    EXPECT_EQ(spot1_it->x, 100);
    EXPECT_EQ(spot1_it->y, 150);
    EXPECT_EQ(spot2_it->x, 200);
    EXPECT_EQ(spot2_it->y, 250);
}

TEST_F(ThermalSpotManagerTest, CoordinateValidation) {
    // Test coordinate validation boundaries
    struct TestCase {
        int x, y;
        bool should_succeed;
        std::string description;
    };
    
    std::vector<TestCase> test_cases = {
        {0, 0, true, "minimum valid coordinates"},
        {639, 479, true, "maximum valid coordinates"},
        {320, 240, true, "center coordinates"},
        {-1, 240, false, "negative X"},
        {320, -1, false, "negative Y"},
        {640, 240, false, "X too large"},
        {320, 480, false, "Y too large"},
        {640, 480, false, "both coordinates too large"}
    };
    
    for (const auto& test_case : test_cases) {
        auto result = manager_->createSpot("test-" + std::to_string(test_case.x) + "-" + std::to_string(test_case.y),
                                         test_case.x, test_case.y);
        
        if (test_case.should_succeed) {
            EXPECT_TRUE(result.success) << "Failed for " << test_case.description;
            // Clean up for next test
            if (result.success) {
                manager_->deleteSpot(result.spot_id);
            }
        } else {
            EXPECT_FALSE(result.success) << "Should have failed for " << test_case.description;
            EXPECT_EQ(result.error_code, RPCErrorCode::INVALID_PARAMS);
        }
    }
}

} // namespace test
} // namespace thermal