#include <gtest/gtest.h>
#include "mqtt/client.h"
#include "thingsboard/device.h"
#include "config/configuration.h"
#include <thread>
#include <chrono>

namespace thermal {

class ConnectionIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test configuration for local broker
        // This test will fail if no local MQTT broker is running
        config.host = "localhost";
        config.port = 1883;
        config.access_token = "integration_test_token";
        config.device_id = "integration_test_device";
        config.use_ssl = false;
        config.keep_alive_seconds = 30;
        config.qos_level = 1;
    }
    
    ThingsBoardConfig config;
};

TEST_F(ConnectionIntegrationTest, BasicMQTTClientConnection) {
    MQTTClient client("tcp://localhost:1883", "integration_test_client");
    
    // Initial state should be disconnected
    EXPECT_FALSE(client.is_connected());
    EXPECT_EQ(client.get_connection_state(), ConnectionState::DISCONNECTED);
    
    // Attempt connection (will likely fail without broker)
    bool connect_result = client.connect();
    EXPECT_TRUE(connect_result);  // Should initiate connection
    
    // Wait a moment for connection attempt
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check final state (likely FAILED due to no broker)
    auto final_state = client.get_connection_state();
    EXPECT_TRUE(final_state == ConnectionState::CONNECTING || 
                final_state == ConnectionState::FAILED ||
                final_state == ConnectionState::CONNECTED);
}

TEST_F(ConnectionIntegrationTest, ThingsBoardDeviceConnectionAttempt) {
    ThingsBoardDevice device(config);
    
    // Initial state
    EXPECT_FALSE(device.is_connected());
    
    // Attempt connection
    bool connect_result = device.connect();
    EXPECT_TRUE(connect_result);  // Should initiate connection
    
    // Wait for connection attempt to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Check state (likely failed due to no broker or invalid token)
    const auto& state = device.get_connection_state();
    EXPECT_NE(state.connection_state, ConnectionState::DISCONNECTED);
}

TEST_F(ConnectionIntegrationTest, TelemetryTransmissionWithoutConnection) {
    ThingsBoardDevice device(config);
    
    // Try to send telemetry without connecting
    bool result = device.send_telemetry(1, 25.5);
    EXPECT_FALSE(result);  // Should fail when not connected
    
    // Error count should be incremented
    const auto& state = device.get_connection_state();
    EXPECT_GT(state.total_errors, 0);
}

TEST_F(ConnectionIntegrationTest, MultipleConnectionAttempts) {
    ThingsBoardDevice device(config);
    
    // Multiple connection attempts should be handled gracefully
    device.connect();
    device.connect();
    device.connect();
    
    // Wait for attempts to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Should not crash
    SUCCEED();
}

TEST_F(ConnectionIntegrationTest, AutoReconnectBehavior) {
    ThingsBoardDevice device(config);
    device.set_auto_reconnect(true);
    
    // Attempt connection
    device.connect();
    
    // Wait for connection attempt
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Check that auto-reconnect doesn't cause crashes
    const auto& state = device.get_connection_state();
    EXPECT_TRUE(state.connection_state == ConnectionState::CONNECTING ||
                state.connection_state == ConnectionState::RECONNECTING ||
                state.connection_state == ConnectionState::FAILED ||
                state.connection_state == ConnectionState::CONNECTED);
}

TEST_F(ConnectionIntegrationTest, GracefulDisconnection) {
    ThingsBoardDevice device(config);
    
    // Connect and then disconnect
    device.connect();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    bool disconnect_result = device.disconnect();
    EXPECT_TRUE(disconnect_result);
    
    // Should be disconnected after disconnect
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(device.is_connected());
}

// Test with invalid broker address
TEST_F(ConnectionIntegrationTest, InvalidBrokerAddress) {
    config.host = "invalid.broker.address";
    config.port = 1883;
    
    ThingsBoardDevice device(config);
    
    bool connect_result = device.connect();
    EXPECT_TRUE(connect_result);  // Should still initiate connection
    
    // Wait for connection to fail
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be in failed state
    const auto& state = device.get_connection_state();
    EXPECT_EQ(state.connection_state, ConnectionState::FAILED);
}

// Test connection statistics tracking
TEST_F(ConnectionIntegrationTest, ConnectionStatisticsTracking) {
    ThingsBoardDevice device(config);
    
    const auto& initial_state = device.get_connection_state();
    EXPECT_EQ(initial_state.total_messages_sent, 0);
    EXPECT_EQ(initial_state.total_errors, 0);
    EXPECT_EQ(initial_state.reconnect_attempts, 0);
    
    // Attempt some operations
    device.connect();
    device.send_telemetry(1, 25.5);  // Will fail due to no connection
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    const auto& final_state = device.get_connection_state();
    EXPECT_GT(final_state.total_errors, 0);  // Should have errors from failed telemetry
}

} // namespace thermal