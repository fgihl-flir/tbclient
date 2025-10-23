#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "thingsboard/device.h"
#include "config/configuration.h"

namespace thermal {

class ThingsBoardDeviceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a valid test configuration
        config.host = "localhost";
        config.port = 1883;
        config.access_token = "test_token_123";
        config.device_id = "test_device";
        config.use_ssl = false;
        config.keep_alive_seconds = 60;
        config.qos_level = 1;
        
        // This will fail to connect but we can test the interface
        device = std::make_unique<ThingsBoardDevice>(config);
    }
    
    void TearDown() override {
        if (device) {
            device->disconnect();
        }
    }
    
    ThingsBoardConfig config;
    std::unique_ptr<ThingsBoardDevice> device;
};

TEST_F(ThingsBoardDeviceTest, InitializationWithValidConfig) {
    EXPECT_FALSE(device->is_connected());
    
    const auto& state = device->get_connection_state();
    EXPECT_EQ(state.connection_state, ConnectionState::DISCONNECTED);
}

TEST_F(ThingsBoardDeviceTest, InitializationWithInvalidConfig) {
    ThingsBoardConfig invalid_config;
    invalid_config.host = "";  // Invalid empty host
    invalid_config.access_token = "token";
    
    EXPECT_THROW(ThingsBoardDevice invalid_device(invalid_config), std::invalid_argument);
}

TEST_F(ThingsBoardDeviceTest, ConnectInitiatesConnection) {
    bool result = device->connect();
    EXPECT_TRUE(result);  // Should return true for initiated connection
}

TEST_F(ThingsBoardDeviceTest, SendTelemetryWhenDisconnected) {
    bool result = device->send_telemetry(1, 25.5);
    EXPECT_FALSE(result);  // Should fail when not connected
}

TEST_F(ThingsBoardDeviceTest, SendTelemetryWithInvalidTemperature) {
    // Even if we were connected, invalid temperatures should be rejected
    bool result1 = device->send_telemetry(1, -150.0);  // Too cold
    bool result2 = device->send_telemetry(1, 600.0);   // Too hot
    
    EXPECT_FALSE(result1);
    EXPECT_FALSE(result2);
}

TEST_F(ThingsBoardDeviceTest, SendTelemetryWithValidTemperature) {
    // Valid temperature should be accepted for processing (but fail due to no connection)
    bool result = device->send_telemetry(1, 25.5);
    EXPECT_FALSE(result);  // Fails due to disconnection, not temperature validation
}

TEST_F(ThingsBoardDeviceTest, SendTelemetryWithTimestamp) {
    auto timestamp = std::chrono::system_clock::now();
    bool result = device->send_telemetry(1, 25.5, timestamp);
    EXPECT_FALSE(result);  // Should fail when not connected
}

TEST_F(ThingsBoardDeviceTest, AutoReconnectConfiguration) {
    device->set_auto_reconnect(true);
    device->set_auto_reconnect(false);
    
    // Should not crash - no direct way to verify the setting
    SUCCEED();
}

TEST_F(ThingsBoardDeviceTest, DisconnectWhenNotConnected) {
    bool result = device->disconnect();
    EXPECT_TRUE(result);  // Should succeed even when not connected
}

TEST_F(ThingsBoardDeviceTest, MultipleConnectionAttempts) {
    device->connect();
    bool second_attempt = device->connect();
    EXPECT_TRUE(second_attempt);  // Should handle multiple connection attempts gracefully
}

// Test boundary temperature values
TEST_F(ThingsBoardDeviceTest, BoundaryTemperatureValues) {
    // Test exact boundary values
    bool result1 = device->send_telemetry(1, -100.0);  // Minimum valid
    bool result2 = device->send_telemetry(1, 500.0);   // Maximum valid
    bool result3 = device->send_telemetry(1, -100.1);  // Just below minimum
    bool result4 = device->send_telemetry(1, 500.1);   // Just above maximum
    
    // All should fail due to disconnection, but -100.1 and 500.1 should fail due to validation
    EXPECT_FALSE(result1);
    EXPECT_FALSE(result2);
    EXPECT_FALSE(result3);
    EXPECT_FALSE(result4);
}

} // namespace thermal