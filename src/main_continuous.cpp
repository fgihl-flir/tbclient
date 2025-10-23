#include "config/configuration.h"
#include "thermal/temperature_reading.h"
#include "thermal/measurement_spot.h"
#include "thingsboard/device.h"
#include "common/logger.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <signal.h>
#include <atomic>

// Global flag for graceful shutdown
std::atomic<bool> g_shutdown_requested{false};

void signal_handler(int signal) {
    g_shutdown_requested = true;
    std::cout << "\nShutdown requested (signal " << signal << ")..." << std::endl;
}

class ContinuousTelemetryApp {
private:
    thermal::Configuration config_;
    std::unique_ptr<thermal::ThingsBoardDevice> device_;
    std::vector<thermal::MeasurementSpot> measurement_spots_;
    std::chrono::steady_clock::time_point last_telemetry_time_;
    int total_transmissions_ = 0;
    int failed_transmissions_ = 0;

public:
    bool initialize(const std::string& config_file) {
        try {
            // Load configuration
            config_.load_from_file(config_file);
            
            LOG_INFO("Configuration loaded successfully");
            LOG_INFO("ThingsBoard host: " << config_.thingsboard_config.host);
            LOG_INFO("MQTT port: " << config_.thingsboard_config.port);
            LOG_INFO("Device ID: " << config_.thingsboard_config.device_id);
            LOG_INFO("Telemetry interval: " << config_.telemetry_config.interval_seconds << " seconds");
            
            // Initialize ThingsBoard device with real MQTT
            device_ = std::make_unique<thermal::ThingsBoardDevice>(config_.thingsboard_config);
            device_->set_auto_reconnect(true);
            
            // Initialize measurement spots
            measurement_spots_ = config_.telemetry_config.measurement_spots;
            
            // Enable only active spots
            for (auto& spot : measurement_spots_) {
                if (spot.enabled) {
                    spot.set_state(thermal::SpotState::ACTIVE);
                    LOG_INFO("Enabled measurement spot: " << spot.name << " (ID: " << spot.id 
                            << ") at (" << spot.x << "," << spot.y << ") range: " 
                            << spot.min_temp << "°C - " << spot.max_temp << "°C");
                }
            }
            
            LOG_INFO("Initialized " << measurement_spots_.size() << " measurement spots");
            
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERROR("Initialization failed: " << e.what());
            return false;
        }
    }
    
    bool connect() {
        LOG_INFO("Connecting to ThingsBoard (simulated)...");
        
        if (!device_->connect()) {
            LOG_ERROR("Failed to initiate connection to ThingsBoard");
            return false;
        }
        
        // Wait for connection to complete
        auto start_time = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(10);
        
        while (!device_->is_connected() && !g_shutdown_requested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (std::chrono::steady_clock::now() - start_time > timeout) {
                LOG_ERROR("Connection to ThingsBoard timed out");
                return false;
            }
        }
        
        if (g_shutdown_requested) {
            return false;
        }
        
        LOG_INFO("Successfully connected to ThingsBoard");
        last_telemetry_time_ = std::chrono::steady_clock::now();
        return true;
    }
    
    void run_continuous_telemetry() {
        LOG_INFO("Starting continuous telemetry transmission...");
        LOG_INFO("Press Ctrl+C to stop gracefully");
        
        while (!g_shutdown_requested) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - last_telemetry_time_).count();
            
            if (elapsed >= config_.telemetry_config.interval_seconds) {
                if (device_->is_connected()) {
                    send_telemetry_batch();
                    last_telemetry_time_ = current_time;
                } else {
                    LOG_WARN("Device not connected, attempting to reconnect...");
                    if (connect()) {
                        LOG_INFO("Reconnection successful, resuming telemetry");
                    } else {
                        LOG_ERROR("Reconnection failed, will retry in next cycle");
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                    }
                }
            }
            
            // Sleep for a short interval to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        LOG_INFO("Continuous telemetry stopped");
    }
    
    void send_telemetry_batch() {
        LOG_INFO("Sending telemetry batch...");
        
        int batch_successes = 0;
        int batch_failures = 0;
        
        for (auto& spot : measurement_spots_) {
            if (!spot.enabled || spot.get_state() != thermal::SpotState::ACTIVE) {
                continue;
            }
            
            // Generate temperature while spot is active
            double temperature = spot.generate_temperature();
            
            // Temporarily set to reading state for telemetry
            spot.set_state(thermal::SpotState::READING);
            
            // Add timestamp for more realistic telemetry
            auto timestamp = std::chrono::system_clock::now();
            bool success = device_->send_telemetry(spot.id, temperature, timestamp);
            
            if (success) {
                LOG_INFO("Spot " << spot.id << " (" << spot.name << "): " 
                        << std::fixed << std::setprecision(2) << temperature << "°C ✓");
                batch_successes++;
                total_transmissions_++;
            } else {
                LOG_WARN("Spot " << spot.id << " (" << spot.name << "): " 
                        << std::fixed << std::setprecision(2) << temperature << "°C ✗");
                batch_failures++;
                failed_transmissions_++;
            }
            
            // Return to active state
            spot.set_state(thermal::SpotState::ACTIVE);
            
            // Small delay between spot readings
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        LOG_INFO("Batch complete: " << batch_successes << " sent, " 
                << batch_failures << " failed");
        
        // Log periodic statistics
        if (total_transmissions_ > 0 && total_transmissions_ % 20 == 0) {
            print_statistics();
        }
    }
    
    void print_statistics() {
        const auto& stats = device_->get_connection_stats();
        
        LOG_INFO("=== Telemetry Statistics ===");
        LOG_INFO("Total transmissions: " << total_transmissions_);
        LOG_INFO("Failed transmissions: " << failed_transmissions_);
        LOG_INFO("Success rate: " << std::fixed << std::setprecision(1) 
                << (total_transmissions_ > 0 ? 
                    100.0 * (total_transmissions_ - failed_transmissions_) / total_transmissions_ : 0.0) 
                << "%");
        LOG_INFO("MQTT messages sent: " << stats.messages_sent);
        LOG_INFO("MQTT connection failures: " << stats.connection_failures);
        LOG_INFO("Connection attempts: " << stats.connection_attempts);
        LOG_INFO("===========================");
    }
    
    void shutdown() {
        LOG_INFO("Shutting down...");
        
        if (device_) {
            print_statistics();
            device_->disconnect();
        }
        
        LOG_INFO("Application shutdown complete");
    }
};

int main() {
    // Setup signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    thermal::Logger::instance();
    
    LOG_INFO("Starting thermal camera MQTT client (User Story 2 - Continuous Telemetry)...");
    
    ContinuousTelemetryApp app;
    
    // Initialize application
    if (!app.initialize("thermal_config.json")) {
        LOG_ERROR("Application initialization failed");
        return 1;
    }
    
    // Connect to ThingsBoard
    if (!app.connect()) {
        LOG_ERROR("Failed to connect to ThingsBoard");
        return 1;
    }
    
    // Run continuous telemetry
    try {
        app.run_continuous_telemetry();
    } catch (const std::exception& e) {
        LOG_ERROR("Application error: " << e.what());
    }
    
    // Graceful shutdown
    app.shutdown();
    
    LOG_INFO("Thermal camera MQTT client stopped");
    return 0;
}