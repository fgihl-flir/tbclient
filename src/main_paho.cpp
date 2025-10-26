#include "config/configuration.h"
#include "thermal/temperature_reading.h"
#include "thermal/measurement_spot.h"
#include "thermal/spot_manager/thermal_spot_manager.h"
#include "thermal/rpc/thermal_rpc_handler.h"
#include "thermal/temperature_source/temperature_source_factory.h"
#include "thingsboard/device.h"
#include "provisioning/workflow.h"
#include "common/logger.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <signal.h>
#include <filesystem>

// Global flag for graceful shutdown
volatile bool keep_running = true;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LOG_INFO("Received shutdown signal, stopping...");
        keep_running = false;
    }
}

int main() {
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    thermal::Logger::instance();
    
    LOG_INFO("Starting thermal camera MQTT client with RPC support (Real Paho MQTT Implementation)...");
    
    try {
        // ==================================================
        // STEP 1: Check for provisioning requirement
        // ==================================================
        std::string base_path = ".";
        
        // Check if provision.txt exists and provisioning should be triggered
        if (std::filesystem::exists(base_path + "/provision.txt")) {
            LOG_INFO("=== Provisioning Mode Detected ===");
            LOG_INFO("Found provision.txt - Starting device provisioning workflow");
            
            // Load provisioning credentials to get broker host/port
            auto provision_creds = config::ProvisioningCredentials::loadFromFile(base_path + "/provision.json");
            if (!provision_creds || !provision_creds->isValid()) {
                LOG_ERROR("✗ Failed to load valid provisioning credentials from provision.json");
                LOG_ERROR("Please ensure provision.json exists and contains valid credentials");
                return 1;
            }
            
            std::string broker_host = provision_creds->getServerUrl();
            int broker_port = provision_creds->getServerPort();
            
            LOG_INFO("ThingsBoard Server: " << broker_host << ":" << broker_port);
            
            // Create provisioning workflow with correct broker details
            provisioning::ProvisioningWorkflow workflow(base_path, broker_host, broker_port);
            
            // Check if thermal_config.json already exists
            std::string config_path = base_path + "/thermal_config.json";
            if (std::filesystem::exists(config_path)) {
                LOG_INFO("Note: thermal_config.json exists but will be updated with new credentials");
            } else {
                LOG_INFO("thermal_config.json will be created with provisioned credentials");
            }
            
            // Execute provisioning
            auto provision_result = workflow.executeProvisioning();
            
            if (provision_result.success) {
                LOG_INFO("✓ Provisioning completed successfully!");
                LOG_INFO("  Device Name: " << provision_result.device_name);
                LOG_INFO("  Access Token: " << provision_result.access_token.substr(0, 8) << "...");
                LOG_INFO("  Duration: " << provision_result.total_duration.count() << " ms");
                LOG_INFO("  Attempts: " << provision_result.attempts_made);
                LOG_INFO("");
                LOG_INFO("thermal_config.json has been created/updated with device credentials");
                LOG_INFO("provision.txt has been marked as processed");
                LOG_INFO("");
                LOG_INFO("You can now restart the application to connect with the new device credentials");
                return 0;
            } else {
                LOG_ERROR("✗ Provisioning failed!");
                LOG_ERROR("  Error: " << provision_result.error_message);
                LOG_ERROR("  Attempts: " << provision_result.attempts_made);
                LOG_ERROR("  Duration: " << provision_result.total_duration.count() << " ms");
                LOG_ERROR("");
                LOG_ERROR("Please check:");
                LOG_ERROR("  1. provision.json contains valid credentials");
                LOG_ERROR("  2. ThingsBoard server is accessible");
                LOG_ERROR("  3. Network connectivity is working");
                return 1;
            }
        }
        
        // ==================================================
        // STEP 2: Normal operation mode (no provisioning)
        // ==================================================
        
        // Load configuration
        thermal::Configuration config;
        config.load_from_file("thermal_config.json");
        
        LOG_INFO("Configuration loaded successfully");
        LOG_INFO("ThingsBoard host: " << config.thingsboard_config.host);
        LOG_INFO("MQTT port: " << config.thingsboard_config.port);
        LOG_INFO("Device ID: " << config.thingsboard_config.device_id);
        
        // Initialize thermal spot manager with temperature source
        auto temp_source = thermal::TemperatureSourceFactory::createDefault();
        auto spot_manager = std::make_shared<thermal::ThermalSpotManager>(
            std::move(temp_source), "thermal_spots.json");
        
        // Initialize thermal RPC handler
        auto thermal_rpc_handler = std::make_shared<thermal::ThermalRPCHandler>(spot_manager);
        
        // Initialize ThingsBoard device with real Paho MQTT
        thermal::ThingsBoardDevice device(config.thingsboard_config);
        device.set_auto_reconnect(true);
        
        // Set up thermal RPC handler
        device.setThermalRPCHandler(thermal_rpc_handler);
        LOG_INFO("Thermal RPC handler configured");
        
        // Connect to ThingsBoard
        LOG_INFO("Connecting to ThingsBoard...");
        if (!device.connect()) {
            LOG_ERROR("Failed to connect to ThingsBoard");
            return 1;
        }
        
        // Wait for connection to establish
        int connection_wait = 0;
        while (!device.is_connected() && connection_wait < 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            connection_wait++;
        }
        
        if (!device.is_connected()) {
            LOG_ERROR("Connection to ThingsBoard failed after waiting");
            return 1;
        }
        
        LOG_INFO("Successfully connected to ThingsBoard");
        
        // Load any existing spots from persistence
        auto existing_spots = spot_manager->listSpots();
        if (!existing_spots.empty()) {
            LOG_INFO("Loaded " << existing_spots.size() << " existing thermal spots from persistence");
            for (const auto& spot : existing_spots) {
                LOG_INFO("  Spot " << spot.id << " at (" << spot.x << ", " << spot.y << ")");
            }
        }
        
        // Initialize measurement spots from config (if any)
        std::vector<thermal::MeasurementSpot> config_spots = config.telemetry_config.measurement_spots;
        for (auto& spot : config_spots) {
            std::string spot_id = std::to_string(spot.id);  // Convert int to string
            bool created = spot_manager->createSpot(spot_id, spot.x, spot.y);
            if (created) {
                LOG_INFO("Created config spot " << spot_id << " (" << spot.name 
                        << ") at (" << spot.x << ", " << spot.y << ")");
            } else {
                LOG_WARN("Failed to create config spot " << spot_id << " (may already exist)");
            }
        }
        
        LOG_INFO("=== Thermal Camera Ready for RPC Commands ===");
        LOG_INFO("Listening for RPC commands on: v1/devices/me/rpc/request/+");
        LOG_INFO("Available commands:");
        LOG_INFO("  - createSpotMeasurement: Create new thermal spot");
        LOG_INFO("  - moveSpotMeasurement: Move existing spot"); 
        LOG_INFO("  - deleteSpotMeasurement: Delete thermal spot");
        LOG_INFO("  - listSpotMeasurements: List all active spots");
        LOG_INFO("  - getSpotTemperature: Get temperature reading");
        LOG_INFO("Press Ctrl+C to stop...");
        LOG_INFO("===============================================");
        
        // Main loop - keep running and send periodic telemetry
        auto last_telemetry = std::chrono::steady_clock::now();
        auto telemetry_interval = std::chrono::seconds(config.telemetry_config.interval_seconds);
        
        while (keep_running) {
            // Check if we should send telemetry
            auto now = std::chrono::steady_clock::now();
            if (now - last_telemetry >= telemetry_interval) {
                // Send telemetry for all active spots from spot manager
                auto active_spots = spot_manager->listSpots();
                for (const auto& spot : active_spots) {
                    std::string spot_id_str = std::to_string(spot.id);  // Convert int to string
                    float temperature = spot_manager->getSpotTemperature(spot_id_str);
                    if (temperature > 0.0f) {
                        bool sent = device.send_telemetry(spot.id, temperature);  // Use int id for telemetry
                        if (sent) {
                            LOG_INFO("Sent telemetry for spot " << spot.id << ": " << std::fixed << std::setprecision(2) << temperature << "°C");
                        } else {
                            LOG_WARN("Failed to send telemetry for spot " << spot.id);
                        }
                    }
                }
                
                // Also send telemetry for original config spots if they exist and aren't managed by spot manager
                if (active_spots.empty()) {
                    for (auto& config_spot : config_spots) {
                        // Generate temperature for config spot
                        config_spot.set_state(thermal::SpotState::ACTIVE);
                        double temperature = config_spot.generate_temperature();
                        
                        bool sent = device.send_telemetry(config_spot.id, temperature);
                        if (sent) {
                            LOG_INFO("Sent config telemetry for spot " << config_spot.id << " (" << config_spot.name 
                                    << "): " << std::fixed << std::setprecision(2) << temperature << "°C");
                        } else {
                            LOG_WARN("Failed to send config telemetry for spot " << config_spot.id);
                        }
                    }
                }
                
                last_telemetry = now;
            }
            
            // Check connection status
            if (!device.is_connected()) {
                LOG_WARN("Lost connection to ThingsBoard, attempting to reconnect...");
                device.connect();
            }
            
            // Sleep for a short time to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Display final statistics
        const auto& stats = device.get_connection_stats();
        LOG_INFO("=== Final Statistics ===");
        LOG_INFO("Connection attempts: " << stats.connection_attempts);
        LOG_INFO("Messages sent: " << stats.messages_sent);
        LOG_INFO("Connection failures: " << stats.connection_failures);
        LOG_INFO("========================");
        
        // Disconnect gracefully
        LOG_INFO("Disconnecting from ThingsBoard...");
        device.disconnect();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error: " << e.what());
        return 1;
    }
    
    LOG_INFO("Thermal camera MQTT client with RPC support completed");
    return 0;
}