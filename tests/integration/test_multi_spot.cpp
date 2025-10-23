#include <gtest/gtest.h>
#include "config/configuration.h"
#include "thingsboard/device.h"
#include "thermal/measurement_spot.h"
#include "common/logger.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>

class MultiSpotIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for test output
        thermal::Logger::instance();
        
        // Create test configuration with multiple spots
        config_.thingsboard_config.host = "localhost";
        config_.thingsboard_config.port = 1883;
        config_.thingsboard_config.access_token = "test_token";
        config_.thingsboard_config.device_id = "test_device";
        config_.thingsboard_config.use_ssl = false;
        config_.thingsboard_config.keep_alive_seconds = 60;
        config_.thingsboard_config.qos_level = 1;
        
        // Create multiple measurement spots with different characteristics
        thermal::MeasurementSpot spot1;
        spot1.id = 1;
        spot1.name = "Center Spot";
        spot1.x = 160;
        spot1.y = 120;
        spot1.min_temp = 20.0;
        spot1.max_temp = 80.0;
        spot1.noise_factor = 0.1;
        spot1.enabled = true;
        
        thermal::MeasurementSpot spot2;
        spot2.id = 2;
        spot2.name = "Hot Zone";
        spot2.x = 50;
        spot2.y = 50;
        spot2.min_temp = 60.0;
        spot2.max_temp = 120.0;
        spot2.noise_factor = 0.15;
        spot2.enabled = true;
        
        thermal::MeasurementSpot spot3;
        spot3.id = 3;
        spot3.name = "Cool Zone";
        spot3.x = 250;
        spot3.y = 180;
        spot3.min_temp = 15.0;
        spot3.max_temp = 40.0;
        spot3.noise_factor = 0.05;
        spot3.enabled = true;
        
        thermal::MeasurementSpot spot4;
        spot4.id = 4;
        spot4.name = "Disabled Spot";
        spot4.x = 300;
        spot4.y = 200;
        spot4.min_temp = 25.0;
        spot4.max_temp = 70.0;
        spot4.noise_factor = 0.2;
        spot4.enabled = false;  // This spot should not send data
        
        config_.telemetry_config.measurement_spots = {spot1, spot2, spot3, spot4};
        config_.telemetry_config.interval_seconds = 5;
        config_.telemetry_config.batch_transmission = false;
        config_.telemetry_config.retry_attempts = 3;
        config_.telemetry_config.retry_delay_ms = 1000;
    }
    
    thermal::Configuration config_;
};

// Test that configuration validates with multiple spots
TEST_F(MultiSpotIntegrationTest, ConfigurationValidatesMultipleSpots) {
    EXPECT_TRUE(config_.validate());
    EXPECT_EQ(config_.telemetry_config.measurement_spots.size(), 4);
    
    // Check that all spots are properly configured
    const auto& spots = config_.telemetry_config.measurement_spots;
    
    EXPECT_EQ(spots[0].id, 1);
    EXPECT_EQ(spots[0].name, "Center Spot");
    EXPECT_TRUE(spots[0].enabled);
    
    EXPECT_EQ(spots[1].id, 2);
    EXPECT_EQ(spots[1].name, "Hot Zone");
    EXPECT_TRUE(spots[1].enabled);
    
    EXPECT_EQ(spots[2].id, 3);
    EXPECT_EQ(spots[2].name, "Cool Zone");
    EXPECT_TRUE(spots[2].enabled);
    
    EXPECT_EQ(spots[3].id, 4);
    EXPECT_EQ(spots[3].name, "Disabled Spot");
    EXPECT_FALSE(spots[3].enabled);
}

// Test that each spot generates unique temperature readings within its range
TEST_F(MultiSpotIntegrationTest, SpotsGenerateUniqueTemperatures) {
    auto spots = config_.telemetry_config.measurement_spots;
    
    // Generate multiple readings for each spot and verify they're in range
    for (auto& spot : spots) {
        if (spot.enabled) {
            // Set spot to active state so it can generate temperatures
            spot.set_state(thermal::SpotState::ACTIVE);
            
            for (int i = 0; i < 10; ++i) {
                double temp = spot.generate_temperature();
                EXPECT_GE(temp, spot.min_temp) << "Spot " << spot.id << " temperature below range";
                EXPECT_LE(temp, spot.max_temp) << "Spot " << spot.id << " temperature above range";
            }
        }
    }
}

// Test spot state management across multiple spots
TEST_F(MultiSpotIntegrationTest, MultipleSpotStateManagement) {
    auto spots = config_.telemetry_config.measurement_spots;
    
    // Test that we can manage state for multiple spots independently
    spots[0].set_state(thermal::SpotState::ACTIVE);
    spots[1].set_state(thermal::SpotState::READING);
    spots[2].set_state(thermal::SpotState::INACTIVE);
    spots[3].set_state(thermal::SpotState::ERROR);
    
    EXPECT_EQ(spots[0].get_state(), thermal::SpotState::ACTIVE);
    EXPECT_EQ(spots[1].get_state(), thermal::SpotState::READING);
    EXPECT_EQ(spots[2].get_state(), thermal::SpotState::INACTIVE);
    EXPECT_EQ(spots[3].get_state(), thermal::SpotState::ERROR);
    
    // Test readiness
    EXPECT_TRUE(spots[0].is_ready());   // Enabled and active
    EXPECT_FALSE(spots[1].is_ready());  // Enabled but reading
    EXPECT_FALSE(spots[2].is_ready());  // Enabled but inactive
    EXPECT_FALSE(spots[3].is_ready());  // Disabled (even though error state)
}

// Test that only enabled spots are considered ready
TEST_F(MultiSpotIntegrationTest, OnlyEnabledSpotsReady) {
    auto spots = config_.telemetry_config.measurement_spots;
    
    // Set all spots to active state
    for (auto& spot : spots) {
        spot.set_state(thermal::SpotState::ACTIVE);
    }
    
    // Count ready spots (should be 3, since spot 4 is disabled)
    int ready_count = 0;
    for (const auto& spot : spots) {
        if (spot.is_ready()) {
            ready_count++;
        }
    }
    
    EXPECT_EQ(ready_count, 3);
}

// Test JSON serialization for multiple spots
TEST_F(MultiSpotIntegrationTest, MultipleSpotJsonSerialization) {
    nlohmann::json spots_json = config_.telemetry_config.to_json();
    
    EXPECT_TRUE(spots_json.contains("measurement_spots"));
    EXPECT_EQ(spots_json["measurement_spots"].size(), 4);
    
    // Verify each spot's JSON representation
    const auto& spots_array = spots_json["measurement_spots"];
    
    EXPECT_EQ(spots_array[0]["id"], 1);
    EXPECT_EQ(spots_array[0]["name"], "Center Spot");
    EXPECT_EQ(spots_array[0]["enabled"], true);
    
    EXPECT_EQ(spots_array[1]["id"], 2);
    EXPECT_EQ(spots_array[1]["name"], "Hot Zone");
    EXPECT_EQ(spots_array[1]["enabled"], true);
    
    EXPECT_EQ(spots_array[2]["id"], 3);
    EXPECT_EQ(spots_array[2]["name"], "Cool Zone");
    EXPECT_EQ(spots_array[2]["enabled"], true);
    
    EXPECT_EQ(spots_array[3]["id"], 4);
    EXPECT_EQ(spots_array[3]["name"], "Disabled Spot");
    EXPECT_EQ(spots_array[3]["enabled"], false);
}

// Test configuration round-trip with multiple spots
TEST_F(MultiSpotIntegrationTest, ConfigurationRoundTripMultipleSpots) {
    // Serialize to JSON
    nlohmann::json config_json = config_.to_json();
    
    // Create new configuration from JSON
    thermal::Configuration new_config;
    new_config.from_json(config_json);
    
    // Verify the configuration matches
    EXPECT_EQ(new_config.telemetry_config.measurement_spots.size(), 4);
    
    const auto& original_spots = config_.telemetry_config.measurement_spots;
    const auto& new_spots = new_config.telemetry_config.measurement_spots;
    
    for (size_t i = 0; i < original_spots.size(); ++i) {
        EXPECT_EQ(new_spots[i].id, original_spots[i].id);
        EXPECT_EQ(new_spots[i].name, original_spots[i].name);
        EXPECT_EQ(new_spots[i].x, original_spots[i].x);
        EXPECT_EQ(new_spots[i].y, original_spots[i].y);
        EXPECT_EQ(new_spots[i].min_temp, original_spots[i].min_temp);
        EXPECT_EQ(new_spots[i].max_temp, original_spots[i].max_temp);
        EXPECT_EQ(new_spots[i].noise_factor, original_spots[i].noise_factor);
        EXPECT_EQ(new_spots[i].enabled, original_spots[i].enabled);
    }
}

// Test duplicate spot ID validation
TEST_F(MultiSpotIntegrationTest, DuplicateSpotIdValidationFails) {
    // Create a configuration with duplicate spot IDs
    auto& spots = config_.telemetry_config.measurement_spots;
    spots[1].id = spots[0].id;  // Make spot 2 have the same ID as spot 1
    
    // Validation should fail
    EXPECT_THROW(config_.validate(), std::invalid_argument);
}

// Test maximum spots limit
TEST_F(MultiSpotIntegrationTest, MaximumSpotsLimit) {
    // Add one more spot to exceed the 5-spot limit
    thermal::MeasurementSpot extra_spot;
    extra_spot.id = 5;
    extra_spot.name = "Extra Spot";
    extra_spot.x = 400;
    extra_spot.y = 300;
    extra_spot.min_temp = 30.0;
    extra_spot.max_temp = 90.0;
    extra_spot.noise_factor = 0.1;
    extra_spot.enabled = true;
    
    config_.telemetry_config.measurement_spots.push_back(extra_spot);
    
    thermal::MeasurementSpot sixth_spot;
    sixth_spot.id = 6;
    sixth_spot.name = "Sixth Spot";
    sixth_spot.x = 500;
    sixth_spot.y = 400;
    sixth_spot.min_temp = 40.0;
    sixth_spot.max_temp = 100.0;
    sixth_spot.noise_factor = 0.1;
    sixth_spot.enabled = true;
    
    config_.telemetry_config.measurement_spots.push_back(sixth_spot);
    
    // Should fail validation with 6 spots
    EXPECT_THROW(config_.validate(), std::invalid_argument);
}

// Test empty spots configuration
TEST_F(MultiSpotIntegrationTest, EmptySpotsConfigurationFails) {
    config_.telemetry_config.measurement_spots.clear();
    
    // Should fail validation with no spots
    EXPECT_THROW(config_.validate(), std::invalid_argument);
}

// Integration test simulating multi-spot telemetry workflow
TEST_F(MultiSpotIntegrationTest, MultiSpotTelemetryWorkflow) {
    // This test simulates the workflow without actual MQTT connection
    auto spots = config_.telemetry_config.measurement_spots;
    
    // Simulate activating spots
    for (auto& spot : spots) {
        if (spot.enabled) {
            spot.set_state(thermal::SpotState::ACTIVE);
        }
    }
    
    // Simulate telemetry cycle
    int telemetry_messages = 0;
    for (auto& spot : spots) {
        if (spot.is_ready()) {
            // Generate temperature while spot is active
            double temperature = spot.generate_temperature();
            
            // Simulate taking a measurement
            spot.set_state(thermal::SpotState::READING);
            
            // Verify temperature is within expected range
            EXPECT_TRUE(spot.is_temperature_expected(temperature));
            
            // Simulate successful telemetry transmission
            telemetry_messages++;
            
            // Return to active state
            spot.set_state(thermal::SpotState::ACTIVE);
        }
    }
    
    // Should have processed 3 enabled spots (spot 4 is disabled)
    EXPECT_EQ(telemetry_messages, 3);
}

// Test different temperature ranges across spots
TEST_F(MultiSpotIntegrationTest, DifferentTemperatureRanges) {
    auto spots = config_.telemetry_config.measurement_spots;
    
    // Set enabled spots to active state
    for (auto& spot : spots) {
        if (spot.enabled) {
            spot.set_state(thermal::SpotState::ACTIVE);
        }
    }
    
    // Generate temperatures for each spot and verify they can be different
    std::vector<double> temperatures;
    for (const auto& spot : spots) {
        if (spot.enabled) {
            double temp = spot.generate_temperature();
            temperatures.push_back(temp);
            
            // Verify temperature is within the specific spot's range
            EXPECT_GE(temp, spot.min_temp);
            EXPECT_LE(temp, spot.max_temp);
        }
    }
    
    // We should have 3 temperatures (from 3 enabled spots)
    EXPECT_EQ(temperatures.size(), 3);
    
    // Verify that spots can have temperatures in their specific ranges
    // This is a probabilistic test - run multiple times
    bool found_different_ranges = false;
    for (int trial = 0; trial < 50 && !found_different_ranges; ++trial) {
        double temp1 = spots[0].generate_temperature(); // 20-80°C range
        double temp2 = spots[1].generate_temperature(); // 60-120°C range  
        
        // If hot zone generates a temp > 80°C and center spot < 60°C, ranges are working
        if (temp2 > 80.0 && temp1 < 60.0) {
            found_different_ranges = true;
        }
    }
    
    // This should succeed with high probability given the different ranges
    EXPECT_TRUE(found_different_ranges) << "Spots should be able to generate temperatures in different ranges";
}