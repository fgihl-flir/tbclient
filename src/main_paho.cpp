#include "config/configuration.h"
#include "thermal/temperature_reading.h"
#include "thermal/measurement_spot.h"
#include "thingsboard/device.h"
#include "common/logger.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

int main() {
    thermal::Logger::instance();
    
    LOG_INFO("Starting thermal camera MQTT client (Real Paho MQTT Implementation)...");
    
    try {
        // Load configuration
        thermal::Configuration config;
        config.load_from_file("thermal_config.json");
        
        LOG_INFO("Configuration loaded successfully");
        LOG_INFO("ThingsBoard host: " << config.thingsboard_config.host);
        LOG_INFO("MQTT port: " << config.thingsboard_config.port);
        LOG_INFO("Device ID: " << config.thingsboard_config.device_id);
        
        // Initialize ThingsBoard device with real Paho MQTT
        thermal::ThingsBoardDevice device(config.thingsboard_config);
        device.set_auto_reconnect(true);
        
        // Connect to ThingsBoard
        LOG_INFO("Connecting to ThingsBoard...");
        if (!device.connect()) {
            LOG_ERROR("Failed to connect to ThingsBoard");
            return 1;
        }
        
        // Wait a moment to ensure connection is established
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        if (!device.is_connected()) {
            LOG_ERROR("Connection to ThingsBoard failed");
            return 1;
        }
        
        LOG_INFO("Successfully connected to ThingsBoard");
        
        // Initialize measurement spots
        std::vector<thermal::MeasurementSpot> spots = config.telemetry_config.measurement_spots;
        
        LOG_INFO("Initialized " << spots.size() << " measurement spots");
        
        // Send telemetry data for each measurement spot
        LOG_INFO("Sending telemetry data...");
        bool all_sent = true;
        
        for (auto& spot : spots) {
            // Enable the spot
            spot.set_state(thermal::SpotState::ACTIVE);
            
            double temperature = spot.generate_temperature();
            
            LOG_INFO("Spot " << spot.id << " (" << spot.name 
                    << "): " << std::fixed << std::setprecision(2) << temperature << "°C");
            
            // Send telemetry to ThingsBoard
            bool sent = device.send_telemetry(spot.id, temperature);
            if (sent) {
                LOG_INFO("✓ Telemetry sent successfully for spot " << spot.id);
            } else {
                LOG_ERROR("✗ Failed to send telemetry for spot " << spot.id);
                all_sent = false;
            }
            
            // Small delay between transmissions
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        
        // Wait for message delivery confirmations
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Display connection statistics
        const auto& stats = device.get_connection_stats();
        LOG_INFO("=== Connection Statistics ===");
        LOG_INFO("Connection attempts: " << stats.connection_attempts);
        LOG_INFO("Messages sent: " << stats.messages_sent);
        LOG_INFO("Connection failures: " << stats.connection_failures);
        LOG_INFO("=============================");
        
        // Disconnect gracefully
        LOG_INFO("Disconnecting from ThingsBoard...");
        device.disconnect();
        
        if (all_sent) {
            LOG_INFO("All telemetry data transmitted successfully using real MQTT");
        } else {
            LOG_WARN("Some telemetry transmissions failed");
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error: " << e.what());
        return 1;
    }
    
    LOG_INFO("Thermal camera MQTT client completed");
    return 0;
}