# Integration Analysis: Thermal Spot RPC Extension

**Date**: 26 October 2025  
**Purpose**: Document integration approach for extending existing thermal components with RPC capabilities

## Existing MeasurementSpot Analysis (T006)

### Current Capabilities
- **JSON serialization**: `to_json()` and `from_json()` methods available
- **State management**: `SpotState` enum (INACTIVE, ACTIVE, READING, ERROR)
- **Temperature simulation**: `generate_temperature()` with configurable ranges
- **Validation**: Coordinate and temperature range validation
- **Configuration**: ID, name, coordinates, min/max temp, noise factor

### Integration Points for RPC
- **Coordinate system**: Already supports x,y coordinates (perfect for RPC)
- **State transitions**: Can be extended for RPC control
- **Temperature generation**: Can be adapted for coordinate-based algorithm
- **JSON interface**: Directly usable for RPC responses

### Required Extensions
- Add RPC-specific metadata (created_timestamp, last_reading)
- Extend coordinate validation for 320x240 resolution
- Add spot creation/deletion lifecycle methods
- Integrate with new TemperatureDataSource interface

## Existing paho_device.cpp RPC Integration (T007)

### Current Capabilities
- **MQTT connectivity**: Established ThingsBoard connection patterns
- **Telemetry publishing**: `send_telemetry()` for spot temperature data
- **Event callbacks**: Connection management and error handling
- **Topic management**: Telemetry topic construction and publishing

### Missing RPC Capabilities
- **RPC subscription**: No subscription to `v1/devices/me/rpc/request/+`
- **Message parsing**: No RPC command JSON parsing
- **Response publishing**: No RPC response handling
- **Command routing**: No method-based command dispatching

### Required Extensions
- Add RPC topic subscription in `connect()` method
- Implement `on_message_received()` callback for RPC commands
- Add RPC response publishing methods
- Integrate with ThermalRPCHandler for command processing

## Integration Architecture

### Component Interaction Flow
```
MQTT RPC Topic → paho_device.cpp → ThermalRPCHandler → ThermalSpotManager → MeasurementSpot
                                                    ↓
                                             TemperatureDataSource
                                                    ↓
                                             JSON Response → MQTT Response Topic
```

### Modified paho_device.cpp Integration
```cpp
// Add to ThingsBoardDevice class:
class ThingsBoardDevice : public MQTTEventCallback {
private:
    std::unique_ptr<ThermalRPCHandler> rpc_handler_;  // NEW
    
public:
    // Existing methods...
    
    // NEW: RPC-specific methods
    void on_message_received(const std::string& topic, const std::string& payload) override;
    bool send_rpc_response(const std::string& request_id, const std::string& response);
    
private:
    void handle_rpc_command(const std::string& request_id, const std::string& command);
    std::string build_rpc_response_topic(const std::string& request_id) const;
};
```

### ThermalSpotManager Integration with MeasurementSpot
```cpp
class ThermalSpotManager {
private:
    std::map<std::string, std::unique_ptr<MeasurementSpot>> spots_;  // Reuse existing class
    std::unique_ptr<TemperatureDataSource> temp_source_;
    
public:
    bool createSpot(const std::string& spotId, int x, int y);
    bool moveSpot(const std::string& spotId, int x, int y);
    bool deleteSpot(const std::string& spotId);
    std::vector<MeasurementSpot> listSpots() const;
};
```

### TemperatureDataSource Integration
```cpp
// New interface that works with existing MeasurementSpot
class CoordinateBasedTemperatureSource : public TemperatureDataSource {
public:
    float getTemperature(int x, int y) override {
        // Coordinate-based algorithm
        float base = calculateBaseTemperature(x, y);
        return base + generateRandomVariation();  // ±0.5°C
    }
    
    // Integrates with MeasurementSpot.generate_temperature()
    void configureMeasurementSpot(MeasurementSpot& spot, int x, int y) {
        float base_temp = getTemperature(x, y);
        spot.min_temp = base_temp - 0.5;
        spot.max_temp = base_temp + 0.5;
        spot.x = x;
        spot.y = y;
    }
};
```

## File Modification Plan

### 1. paho_device.cpp Extensions
- Add RPC topic subscription: `v1/devices/me/rpc/request/+`
- Add `on_message_received()` callback implementation
- Add RPC response publishing methods
- Integrate ThermalRPCHandler instantiation and command routing

### 2. MeasurementSpot Extensions
- Add `created_timestamp` and `last_reading` fields to JSON serialization
- Extend coordinate validation for 320x240 bounds
- Add RPC-specific state management methods
- Maintain backward compatibility with existing functionality

### 3. New Component Integration
- ThermalSpotManager orchestrates MeasurementSpot instances
- ThermalRPCHandler parses commands and generates responses
- TemperatureDataSource provides coordinate-based temperature calculation
- All components reuse existing patterns and infrastructure

## Constitution Compliance

✅ **Reuse Existing Infrastructure**: Extends MeasurementSpot and paho_device rather than rewriting  
✅ **Minimal New Abstractions**: Only adds necessary RPC and temperature source interfaces  
✅ **C++17 Patterns**: Smart pointers, RAII, and exception handling maintained  
✅ **MQTT Library Constraint**: Uses existing Eclipse Paho MQTT C++ integration  
✅ **Memory Management**: No dynamic allocation during message processing  

## Implementation Priority

1. **T009-T013**: Core interfaces and data structures
2. **T014-T015**: paho_device.cpp RPC subscription and parsing
3. **T016-T018**: RPC command processing and validation
4. **Integration**: Connect all components through ThermalSpotManager

This integration approach maximizes reuse of existing, well-tested components while adding minimal complexity to achieve RPC control functionality.