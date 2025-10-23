#include <gtest/gtest.h>
#include "thermal/measurement_spot.h"
#include <nlohmann/json.hpp>
#include <stdexcept>

class MeasurementSpotTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up a valid measurement spot for testing
        valid_spot_.id = 1;
        valid_spot_.name = "Test Spot";
        valid_spot_.x = 100;
        valid_spot_.y = 200;
        valid_spot_.min_temp = 20.0;
        valid_spot_.max_temp = 80.0;
        valid_spot_.noise_factor = 0.1;
        valid_spot_.enabled = true;
        valid_spot_.state = thermal::SpotState::ACTIVE;
    }
    
    thermal::MeasurementSpot valid_spot_;
};

// Test validation of valid spot configuration
TEST_F(MeasurementSpotTest, ValidateValidSpot) {
    EXPECT_TRUE(valid_spot_.validate());
}

// Test validation failures for invalid configurations
TEST_F(MeasurementSpotTest, ValidateInvalidId) {
    valid_spot_.id = 0;
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
    
    valid_spot_.id = -1;
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
}

TEST_F(MeasurementSpotTest, ValidateEmptyName) {
    valid_spot_.name = "";
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
}

TEST_F(MeasurementSpotTest, ValidateInvalidNameCharacters) {
    valid_spot_.name = "Invalid@Name";
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
    
    valid_spot_.name = "Valid_Name-123";  // This should be valid
    EXPECT_TRUE(valid_spot_.validate());
}

TEST_F(MeasurementSpotTest, ValidateNegativeCoordinates) {
    valid_spot_.x = -1;
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
    
    valid_spot_.x = 100;
    valid_spot_.y = -1;
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
}

TEST_F(MeasurementSpotTest, ValidateTemperatureRange) {
    // Min >= Max should fail
    valid_spot_.min_temp = 80.0;
    valid_spot_.max_temp = 20.0;
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
    
    // Equal temperatures should fail
    valid_spot_.min_temp = 50.0;
    valid_spot_.max_temp = 50.0;
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
}

TEST_F(MeasurementSpotTest, ValidateExtremeTemperatures) {
    // Below minimum range
    valid_spot_.min_temp = -150.0;
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
    
    // Above maximum range
    valid_spot_.min_temp = 20.0;
    valid_spot_.max_temp = 600.0;
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
}

TEST_F(MeasurementSpotTest, ValidateNoiseFactor) {
    // Below minimum
    valid_spot_.noise_factor = -0.1;
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
    
    // Above maximum
    valid_spot_.noise_factor = 1.1;
    EXPECT_THROW(valid_spot_.validate(), std::invalid_argument);
    
    // Edge cases should be valid
    valid_spot_.noise_factor = 0.0;
    EXPECT_TRUE(valid_spot_.validate());
    
    valid_spot_.noise_factor = 1.0;
    EXPECT_TRUE(valid_spot_.validate());
}

// Test JSON serialization and deserialization
TEST_F(MeasurementSpotTest, JsonSerialization) {
    nlohmann::json json_data = valid_spot_.to_json();
    
    EXPECT_EQ(json_data["id"], 1);
    EXPECT_EQ(json_data["name"], "Test Spot");
    EXPECT_EQ(json_data["x"], 100);
    EXPECT_EQ(json_data["y"], 200);
    EXPECT_EQ(json_data["min_temp"], 20.0);
    EXPECT_EQ(json_data["max_temp"], 80.0);
    EXPECT_EQ(json_data["noise_factor"], 0.1);
    EXPECT_EQ(json_data["enabled"], true);
}

TEST_F(MeasurementSpotTest, JsonDeserialization) {
    nlohmann::json json_data = {
        {"id", 2},
        {"name", "JSON Spot"},
        {"x", 150},
        {"y", 250},
        {"min_temp", 30.0},
        {"max_temp", 90.0},
        {"noise_factor", 0.2},
        {"enabled", false}
    };
    
    thermal::MeasurementSpot spot;
    spot.from_json(json_data);
    
    EXPECT_EQ(spot.id, 2);
    EXPECT_EQ(spot.name, "JSON Spot");
    EXPECT_EQ(spot.x, 150);
    EXPECT_EQ(spot.y, 250);
    EXPECT_EQ(spot.min_temp, 30.0);
    EXPECT_EQ(spot.max_temp, 90.0);
    EXPECT_EQ(spot.noise_factor, 0.2);
    EXPECT_EQ(spot.enabled, false);
}

TEST_F(MeasurementSpotTest, JsonRoundTrip) {
    nlohmann::json json_data = valid_spot_.to_json();
    
    thermal::MeasurementSpot new_spot;
    new_spot.from_json(json_data);
    
    EXPECT_EQ(new_spot.id, valid_spot_.id);
    EXPECT_EQ(new_spot.name, valid_spot_.name);
    EXPECT_EQ(new_spot.x, valid_spot_.x);
    EXPECT_EQ(new_spot.y, valid_spot_.y);
    EXPECT_EQ(new_spot.min_temp, valid_spot_.min_temp);
    EXPECT_EQ(new_spot.max_temp, valid_spot_.max_temp);
    EXPECT_EQ(new_spot.noise_factor, valid_spot_.noise_factor);
    EXPECT_EQ(new_spot.enabled, valid_spot_.enabled);
}

// Test temperature generation
TEST_F(MeasurementSpotTest, TemperatureGeneration) {
    // Generate multiple temperatures and check they're in range
    for (int i = 0; i < 100; ++i) {
        double temp = valid_spot_.generate_temperature();
        EXPECT_GE(temp, valid_spot_.min_temp);
        EXPECT_LE(temp, valid_spot_.max_temp);
    }
}

TEST_F(MeasurementSpotTest, TemperatureExpectedRange) {
    EXPECT_TRUE(valid_spot_.is_temperature_expected(25.0));  // Within range
    EXPECT_TRUE(valid_spot_.is_temperature_expected(20.0));  // Min boundary
    EXPECT_TRUE(valid_spot_.is_temperature_expected(80.0));  // Max boundary
    
    EXPECT_FALSE(valid_spot_.is_temperature_expected(19.0)); // Below range
    EXPECT_FALSE(valid_spot_.is_temperature_expected(81.0)); // Above range
}

// Test state management
TEST_F(MeasurementSpotTest, StateManagement) {
    EXPECT_EQ(valid_spot_.get_state(), thermal::SpotState::ACTIVE);
    
    valid_spot_.set_state(thermal::SpotState::READING);
    EXPECT_EQ(valid_spot_.get_state(), thermal::SpotState::READING);
    
    valid_spot_.set_state(thermal::SpotState::INACTIVE);
    EXPECT_EQ(valid_spot_.get_state(), thermal::SpotState::INACTIVE);
    
    valid_spot_.set_state(thermal::SpotState::ERROR);
    EXPECT_EQ(valid_spot_.get_state(), thermal::SpotState::ERROR);
}

TEST_F(MeasurementSpotTest, IsReady) {
    // Enabled and active = ready
    valid_spot_.enabled = true;
    valid_spot_.set_state(thermal::SpotState::ACTIVE);
    EXPECT_TRUE(valid_spot_.is_ready());
    
    // Disabled = not ready
    valid_spot_.enabled = false;
    EXPECT_FALSE(valid_spot_.is_ready());
    
    // Enabled but not active = not ready
    valid_spot_.enabled = true;
    valid_spot_.set_state(thermal::SpotState::INACTIVE);
    EXPECT_FALSE(valid_spot_.is_ready());
    
    valid_spot_.set_state(thermal::SpotState::READING);
    EXPECT_FALSE(valid_spot_.is_ready());
    
    valid_spot_.set_state(thermal::SpotState::ERROR);
    EXPECT_FALSE(valid_spot_.is_ready());
}

// Test edge cases and boundary conditions
TEST_F(MeasurementSpotTest, BoundaryTemperatures) {
    // Test with extreme but valid ranges
    valid_spot_.min_temp = -100.0;
    valid_spot_.max_temp = 500.0;
    EXPECT_TRUE(valid_spot_.validate());
    
    double temp = valid_spot_.generate_temperature();
    EXPECT_GE(temp, -100.0);
    EXPECT_LE(temp, 500.0);
}

TEST_F(MeasurementSpotTest, ZeroNoiseFactor) {
    valid_spot_.noise_factor = 0.0;
    EXPECT_TRUE(valid_spot_.validate());
    
    // With zero noise, all temperatures should be similar
    double temp1 = valid_spot_.generate_temperature();
    double temp2 = valid_spot_.generate_temperature();
    double temp3 = valid_spot_.generate_temperature();
    
    // Should still be within valid range
    EXPECT_GE(temp1, valid_spot_.min_temp);
    EXPECT_LE(temp1, valid_spot_.max_temp);
    EXPECT_GE(temp2, valid_spot_.min_temp);
    EXPECT_LE(temp2, valid_spot_.max_temp);
    EXPECT_GE(temp3, valid_spot_.min_temp);
    EXPECT_LE(temp3, valid_spot_.max_temp);
}

TEST_F(MeasurementSpotTest, MaxNoiseFactor) {
    valid_spot_.noise_factor = 1.0;
    EXPECT_TRUE(valid_spot_.validate());
    
    // With maximum noise, should still generate valid temperatures
    for (int i = 0; i < 50; ++i) {
        double temp = valid_spot_.generate_temperature();
        EXPECT_GE(temp, valid_spot_.min_temp);
        EXPECT_LE(temp, valid_spot_.max_temp);
    }
}

TEST_F(MeasurementSpotTest, LargeCoordinates) {
    valid_spot_.x = 9999;
    valid_spot_.y = 9999;
    EXPECT_TRUE(valid_spot_.validate());
}

TEST_F(MeasurementSpotTest, PartialJsonDeserialization) {
    // Test that missing fields don't crash and use defaults
    nlohmann::json partial_json = {
        {"id", 5},
        {"name", "Partial Spot"}
        // Missing other fields
    };
    
    thermal::MeasurementSpot spot;
    EXPECT_NO_THROW(spot.from_json(partial_json));
    
    EXPECT_EQ(spot.id, 5);
    EXPECT_EQ(spot.name, "Partial Spot");
    // Other fields should have default values
}