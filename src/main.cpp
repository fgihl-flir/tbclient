#include "config/configuration.h"
#include "thermal/temperature_reading.h"
#include "thermal/measurement_spot.h"
#include "thingsboard/mock_device.h"
#include "common/logger.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    thermal::Logger::instance();
    
    LOG_INFO("Starting thermal camera MQTT client (User Story 1 - Mock Implementation)...");
    
    try {
        // Load configuration
        thermal::Configuration config;
        config.load_from_file("thermal_config.json");
        
        LOG_INFO("Configuration loaded successfully");
        LOG_INFO("ThingsBoard host: " << config.thingsboard_config.host);
        LOG_INFO("MQTT port: " << config.thingsboard_config.port);
        LOG_INFO("Device ID: " << config.thingsboard_config.device_id);
        
        // Initialize ThingsBoard device with mock implementation
        thermal::MockThingsBoardDevice device(config.thingsboard_config);
        device.set_auto_reconnect(true);
        device.set_simulation_mode(false, 0); // Disable failures for demo
        
        // Connect to ThingsBoard
        LOG_INFO("Connecting to ThingsBoard (simulated)...");
        if (!device.connect()) {
            LOG_ERROR("Failed to connect to ThingsBoard (simulated)");
            return 1;
        }
        
        // Wait for connection to complete (mock implementation is asynchronous)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        if (!device.is_connected()) {
            LOG_ERROR("Connection to ThingsBoard timed out (simulated)");
            return 1;
        }
        
        LOG_INFO("Successfully connected to ThingsBoard (simulated)");
        
        // Initialize measurement spots
        std::vector<thermal::MeasurementSpot> spots = config.telemetry_config.measurement_spots;
        
        LOG_INFO("Initialized " << spots.size() << " measurement spots");
        
        // Send telemetry data for each measurement spot
        LOG_INFO("Sending telemetry data (simulated)...");
        bool all_sent = true;
        
        for (auto& spot : spots) {
            double temperature = spot.generate_temperature();
            
            LOG_INFO("Spot " << spot.id << " (" << spot.name 
                    << "): " << temperature << "°C");
            
            // Send telemetry to ThingsBoard
            bool sent = device.send_telemetry(spot.id, temperature);
            if (sent) {
                LOG_DEBUG("Telemetry sent for spot " << spot.id);
            } else {
                LOG_WARN("Failed to send telemetry for spot " << spot.id);
                all_sent = false;
            }
            
            // Small delay between transmissions
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Display connection statistics
        const auto& state = device.get_connection_state();
        LOG_INFO("Connection statistics:");
        LOG_INFO("  Messages sent: " << state.total_messages_sent);
        LOG_INFO("  Total errors: " << state.total_errors);
        LOG_INFO("  Reconnect attempts: " << state.reconnect_attempts);
        
        // Wait a moment for any pending messages
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Disconnect gracefully
        LOG_INFO("Disconnecting from ThingsBoard (simulated)...");
        device.disconnect();
        
        if (all_sent) {
            LOG_INFO("All telemetry data transmitted successfully (simulated)");
        } else {
            LOG_WARN("Some telemetry transmissions failed (simulated)");
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error: " << e.what());
        return 1;
    }
    
    LOG_INFO("Thermal camera MQTT client completed");
    return 0;
}