#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mqtt/client.h"
#include <thread>
#include <chrono>

namespace thermal {

class MockMQTTCallback : public MQTTClientCallback {
public:
    MOCK_METHOD(void, on_connection_lost, (const std::string& cause), (override));
    MOCK_METHOD(void, on_message_delivered, (const std::string& topic), (override));
    MOCK_METHOD(void, on_connection_success, (), (override));
    MOCK_METHOD(void, on_connection_failure, (const MQTTError& error), (override));
    MOCK_METHOD(void, on_disconnected, (), (override));
};

class MQTTClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a local test broker URL - will fail but we can test the interface
        test_client = std::make_unique<MQTTClient>("tcp://localhost:1883", "test_client", &mock_callback);
    }
    
    void TearDown() override {
        if (test_client) {
            test_client->disconnect();
        }
    }
    
    std::unique_ptr<MQTTClient> test_client;
    MockMQTTCallback mock_callback;
};

TEST_F(MQTTClientTest, InitialStateIsDisconnected) {
    EXPECT_FALSE(test_client->is_connected());
    EXPECT_EQ(test_client->get_connection_state(), ConnectionState::DISCONNECTED);
}

TEST_F(MQTTClientTest, ConnectInitiatesConnection) {
    // This will fail because there's no broker, but should initiate the attempt
    bool result = test_client->connect("test_user", "test_pass");
    EXPECT_TRUE(result);  // Should return true for initiated connection
    
    // State should change to CONNECTING
    EXPECT_EQ(test_client->get_connection_state(), ConnectionState::CONNECTING);
}

TEST_F(MQTTClientTest, PublishFailsWhenDisconnected) {
    bool result = test_client->publish("test/topic", "test message");
    EXPECT_FALSE(result);
    
    // Should have recorded an error
    auto last_error = test_client->get_last_error();
    EXPECT_NE(last_error, nullptr);
    EXPECT_TRUE(last_error->retry_possible);
}

TEST_F(MQTTClientTest, AutoReconnectConfiguration) {
    test_client->set_auto_reconnect(true, 500, 5000, 3);
    
    // Configuration should be accepted (no direct way to verify settings)
    // This mainly tests that the method doesn't crash
    SUCCEED();
}

TEST_F(MQTTClientTest, ClientStateTracking) {
    const auto& state = test_client->get_state();
    
    EXPECT_EQ(state.connection_state, ConnectionState::DISCONNECTED);
    EXPECT_EQ(state.reconnect_attempts, 0);
    EXPECT_EQ(state.total_messages_sent, 0);
    EXPECT_EQ(state.total_errors, 0);
}

TEST_F(MQTTClientTest, DisconnectWhenNotConnected) {
    bool result = test_client->disconnect();
    EXPECT_TRUE(result);  // Should succeed even when not connected
}

// Test connection with invalid parameters
TEST_F(MQTTClientTest, ConnectionWithEmptyCredentials) {
    bool result = test_client->connect("", "");
    EXPECT_TRUE(result);  // Should still initiate connection attempt
}

// Test multiple publish attempts while disconnected
TEST_F(MQTTClientTest, MultiplePublishAttemptsWhileDisconnected) {
    const auto& initial_state = test_client->get_state();
    int initial_errors = initial_state.total_errors;
    
    test_client->publish("topic1", "message1");
    test_client->publish("topic2", "message2");
    
    const auto& final_state = test_client->get_state();
    EXPECT_GT(final_state.total_errors, initial_errors);
}

} // namespace thermal