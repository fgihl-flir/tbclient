#include <gtest/gtest.h>
#include "thermal/spot_manager/thermal_spot_manager.h"
#include "thermal/temperature_source/temperature_source_factory.h"
#include <filesystem>
#include <fstream>

namespace thermal {

/**
 * @brief Integration test for complete thermal simulator extension
 * 
 * Validates that the extended thermal simulator can:
 * - Dynamically create, move, and delete spots
 * - Persist spot configurations
 * - Provide temperature readings
 * - Handle modular temperature data sources
 * - Maintain state across operations
 */
class SimulatorExtensionIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_persistence_file_ = "test_thermal_spots_integration.json";
        
        // Clean up any existing test file
        if (std::filesystem::exists(test_persistence_file_)) {
            std::filesystem::remove(test_persistence_file_);
        }
        
        // Create manager with temperature source
        auto temp_source = TemperatureSourceFactory::createDefault();
        manager_ = std::make_unique<ThermalSpotManager>(std::move(temp_source), test_persistence_file_);
    }
    
    void TearDown() override {
        manager_.reset();
        
        // Clean up test file
        if (std::filesystem::exists(test_persistence_file_)) {
            std::filesystem::remove(test_persistence_file_);
        }
    }
    
    std::unique_ptr<ThermalSpotManager> manager_;
    std::string test_persistence_file_;
};

TEST_F(SimulatorExtensionIntegrationTest, CompleteSpotLifecycle) {
    // Initially no spots
    auto spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 0);
    
    // Create first spot
    bool result1 = manager_->createSpot("1", 100, 150);
    ASSERT_TRUE(result1);
    
    // Verify spot creation
    spots = manager_->listSpots();
    ASSERT_EQ(spots.size(), 1);
    EXPECT_EQ(spots[0].id, "1");
    EXPECT_EQ(spots[0].x, 100);
    EXPECT_EQ(spots[0].y, 150);
    
    // Get temperature reading
    float temp = manager_->getSpotTemperature("1");
    EXPECT_GT(temp, 0.0f);  // Should have valid temperature
    
    // Move the spot
    bool move_result = manager_->moveSpot("1", 200, 250);
    ASSERT_TRUE(move_result);
    
    // Verify spot movement
    spots = manager_->listSpots();
    ASSERT_EQ(spots.size(), 1);
    EXPECT_EQ(spots[0].x, 200);
    EXPECT_EQ(spots[0].y, 250);
    
    // Temperature should still be available
    temp = manager_->getSpotTemperature("1");
    EXPECT_GT(temp, 0.0f);
    
    // Create additional spots
    ASSERT_TRUE(manager_->createSpot("2", 300, 100));
    ASSERT_TRUE(manager_->createSpot("3", 50, 200));
    
    // Verify multiple spots
    spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 3);
    
    // Delete a spot
    bool delete_result = manager_->deleteSpot("2");
    ASSERT_TRUE(delete_result);
    
    // Verify deletion
    spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 2);
    
    // Verify remaining spots are correct
    bool found_spot_1 = false, found_spot_3 = false;
    for (const auto& spot : spots) {
        if (spot.id == "1") {
            found_spot_1 = true;
            EXPECT_EQ(spot.x, 200);  // Should have moved coordinates
            EXPECT_EQ(spot.y, 250);
        } else if (spot.id == "3") {
            found_spot_3 = true;
            EXPECT_EQ(spot.x, 50);
            EXPECT_EQ(spot.y, 200);
        }
    }
    EXPECT_TRUE(found_spot_1);
    EXPECT_TRUE(found_spot_3);
}

TEST_F(SimulatorExtensionIntegrationTest, PersistenceAcrossRestarts) {
    // Create spots
    ASSERT_TRUE(manager_->createSpot("1", 100, 150));
    ASSERT_TRUE(manager_->createSpot("2", 200, 250));
    
    // Verify persistence file is created
    EXPECT_TRUE(std::filesystem::exists(test_persistence_file_));
    
    // Destroy and recreate manager (simulating restart)
    manager_.reset();
    
    auto temp_source = TemperatureSourceFactory::createDefault();
    manager_ = std::make_unique<ThermalSpotManager>(std::move(temp_source), test_persistence_file_);
    
    // Verify spots were restored
    auto spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 2);
    
    bool found_spot_1 = false, found_spot_2 = false;
    for (const auto& spot : spots) {
        if (spot.id == "1") {
            found_spot_1 = true;
            EXPECT_EQ(spot.x, 100);
            EXPECT_EQ(spot.y, 150);
        } else if (spot.id == "2") {
            found_spot_2 = true;
            EXPECT_EQ(spot.x, 200);
            EXPECT_EQ(spot.y, 250);
        }
    }
    EXPECT_TRUE(found_spot_1);
    EXPECT_TRUE(found_spot_2);
}

TEST_F(SimulatorExtensionIntegrationTest, TemperatureSourceModularity) {
    // Create spot with default temperature source
    ASSERT_TRUE(manager_->createSpot("1", 100, 150));
    
    // Get temperature reading
    float temp1 = manager_->getSpotTemperature("1");
    EXPECT_GT(temp1, 0.0f);
    
    // Temperature should be consistent for same coordinates
    float temp2 = manager_->getSpotTemperature("1");
    EXPECT_NEAR(temp1, temp2, 1.0f);  // Within 1°C (allowing for noise)
    
    // Different coordinates should give different temperatures
    ASSERT_TRUE(manager_->createSpot("2", 300, 50));
    float temp3 = manager_->getSpotTemperature("2");
    EXPECT_GT(temp3, 0.0f);
    
    // Temperature difference should be noticeable (coordinate-based algorithm)
    EXPECT_NE(temp1, temp3);  // Different coordinates should give different base temps
}

TEST_F(SimulatorExtensionIntegrationTest, MaximumSpotsEnforcement) {
    // Create maximum allowed spots (5)
    for (int i = 1; i <= 5; ++i) {
        bool result = manager_->createSpot(std::to_string(i), 100 + i * 10, 150 + i * 10);
        ASSERT_TRUE(result) << "Failed to create spot " << i;
    }
    
    // Verify all spots created
    auto spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 5);
    
    // Try to create 6th spot - should fail
    bool result = manager_->createSpot("6", 200, 250);
    EXPECT_FALSE(result);
    
    // Spot count should remain at 5
    spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 5);
    
    // Delete one spot
    ASSERT_TRUE(manager_->deleteSpot("3"));
    
    // Now should be able to create a new spot
    EXPECT_TRUE(manager_->createSpot("6", 200, 250));
    
    spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 5);
}

TEST_F(SimulatorExtensionIntegrationTest, ErrorHandlingAndValidation) {
    // Test invalid coordinates
    EXPECT_FALSE(manager_->createSpot("1", -10, 150));  // Invalid X
    EXPECT_FALSE(manager_->createSpot("1", 100, -20));  // Invalid Y
    EXPECT_FALSE(manager_->createSpot("1", 1000, 150)); // X too large
    EXPECT_FALSE(manager_->createSpot("1", 100, 1000)); // Y too large
    
    // Create a valid spot
    ASSERT_TRUE(manager_->createSpot("1", 100, 150));
    
    // Test duplicate ID
    EXPECT_FALSE(manager_->createSpot("1", 200, 250));
    
    // Test operations on non-existent spots
    EXPECT_FALSE(manager_->moveSpot("999", 100, 150));
    EXPECT_FALSE(manager_->deleteSpot("999"));
    
    // Test invalid spot ID format (should still work with string IDs)
    EXPECT_TRUE(manager_->createSpot("test-spot", 200, 250));
    
    // Temperature reading on non-existent spot should return 0 or handle gracefully
    float temp = manager_->getSpotTemperature("999");
    EXPECT_EQ(temp, 0.0f);  // Or whatever the error value is
}

TEST_F(SimulatorExtensionIntegrationTest, ConcurrentOperationsSequential) {
    // Test that operations work correctly when performed in sequence
    // (This simulates what would happen with RPC commands)
    
    // Rapid creation
    for (int i = 1; i <= 3; ++i) {
        EXPECT_TRUE(manager_->createSpot(std::to_string(i), 100 + i * 20, 150 + i * 20));
    }
    
    // Rapid movement
    for (int i = 1; i <= 3; ++i) {
        EXPECT_TRUE(manager_->moveSpot(std::to_string(i), 200 + i * 20, 250 + i * 20));
    }
    
    // Rapid temperature readings
    for (int i = 1; i <= 3; ++i) {
        float temp = manager_->getSpotTemperature(std::to_string(i));
        EXPECT_GT(temp, 0.0f);
    }
    
    // Rapid deletion
    for (int i = 1; i <= 3; ++i) {
        EXPECT_TRUE(manager_->deleteSpot(std::to_string(i)));
    }
    
    // Verify all spots removed
    auto spots = manager_->listSpots();
    EXPECT_EQ(spots.size(), 0);
}

} // namespace thermal