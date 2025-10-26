# Research: Thermal Camera Spot Measurement RPC Control

**Date**: 26 October 2025  
**Feature**: Thermal Camera Spot Measurement RPC Control  
**Purpose**: Resolve technical unknowns and establish implementation decisions

## Executive Summary

Research confirms feasibility of extending existing thermal measurement infrastructure with ThingsBoard RPC control. Key findings: 
- Existing `MeasurementSpot` and `TemperatureReading` classes provide solid foundation
- ThingsBoard RPC patterns documented in control.md are ready for implementation  
- Modular architecture design enables future remote data source integration
- JSON persistence and coordinate-based temperature algorithms are straightforward to implement

## Research Tasks Completed

### 1. Existing Thermal Simulator Architecture Analysis

**Question**: How does existing thermal simulator integrate with new RPC handlers?

**Decision**: Direct integration via extended `MeasurementSpot` class and new `ThermalSpotManager`

**Rationale**: 
- Existing `src/thermal/measurement_spot.cpp` already provides spot management with JSON serialization
- Current `MeasurementSpot` class has coordinates (x,y), temperature generation, and state management
- Well-designed `SpotState` enum (INACTIVE, ACTIVE, READING, ERROR) fits RPC control model
- Existing validation and error handling patterns can be reused

**Alternatives considered**:
- Complete rewrite: Rejected due to unnecessary complexity and constitution violation
- Separate RPC layer: Rejected as existing MeasurementSpot already has required functionality

**Implementation approach**: Extend existing classes, add RPC command handlers as new layer

---

### 2. Current Thermal Simulator Interface Analysis  

**Question**: What is the current thermal simulator architecture and interface?

**Decision**: Leverage existing `thermal::MeasurementSpot` and `thermal::TemperatureReading` infrastructure

**Rationale**:
- `MeasurementSpot` class provides: coordinates, temperature generation, state management, JSON serialization
- `TemperatureReading` class provides: quality indicators, timestamp handling, validation
- Both classes follow constitution requirements (C++17, smart pointers, exception handling)
- Existing test coverage provides stability foundation

**Alternatives considered**:
- Create new thermal interface: Rejected due to code duplication
- Use external thermal library: Rejected due to constitution (simplicity principle)

**Integration pattern**: RPC handlers → ThermalSpotManager → MeasurementSpot instances

---

### 3. Coordinate-Based Temperature Algorithm Design

**Question**: How should coordinate-based temperature algorithm be implemented?

**Decision**: Implement `CoordinateBasedTemperatureSource` with configurable base temperature calculation

**Rationale**: 
- Separates algorithm logic from spot management (single responsibility)
- Allows different temperature distribution patterns (linear, radial, noise-based)
- Supports future integration with remote temperature sources
- Maintains ±0.5°C random variation requirement from clarifications

**Algorithm**: 
```cpp
float getTemperature(int x, int y) {
    // Base temperature using distance from center
    float center_x = 160.0f, center_y = 120.0f;  // Image center
    float distance = sqrt(pow(x - center_x, 2) + pow(y - center_y, 2));
    float base_temp = 20.0f + (distance / 320.0f) * 30.0f;  // 20-50°C range
    
    // Random variation ±0.5°C
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
    
    return base_temp + dist(gen);
}
```

**Alternatives considered**:
- Fixed temperature per coordinate: Rejected as unrealistic for thermal simulation
- Complex environmental simulation: Rejected due to proof-of-concept simplicity requirement

---

### 4. JSON Schema Design for Spot Persistence

**Question**: What JSON schema should be used for spot persistence?

**Decision**: Use existing `MeasurementSpot::to_json()` format with extensions for RPC-specific data

**Schema**:
```json
{
  "thermal_spots": {
    "version": "1.0",
    "spots": [
      {
        "id": 1,
        "name": "spot_1", 
        "x": 160,
        "y": 120,
        "min_temp": 19.5,
        "max_temp": 50.5,
        "noise_factor": 0.1,
        "enabled": true,
        "created_timestamp": "2025-10-26T10:30:00Z",
        "last_temperature": 25.3
      }
    ],
    "last_updated": "2025-10-26T10:35:00Z"
  }
}
```

**Rationale**:
- Reuses existing `MeasurementSpot::to_json()` method (constitution: simplicity)
- Adds RPC-specific metadata (timestamps, last reading)
- Version field supports future schema evolution
- Graceful degradation: invalid spots skipped, not entire file rejected

**Alternatives considered**:
- Binary format: Rejected due to debugging difficulty and simplicity violation
- XML format: Rejected due to parsing complexity and size overhead

---

### 5. Modular Temperature Data Source Interface Design

**Question**: How should modular temperature data source interface be designed for future remote integration?

**Decision**: Simple callback interface `float getTemperature(int x, int y)` with factory pattern

**Interface Design**:
```cpp
namespace thermal {
    class TemperatureDataSource {
    public:
        virtual ~TemperatureDataSource() = default;
        virtual float getTemperature(int x, int y) = 0;
        virtual bool isReady() const = 0;
        virtual std::string getSourceName() const = 0;
    };
    
    class CoordinateBasedTemperatureSource : public TemperatureDataSource {
        // Current simulator implementation
    };
    
    class RemoteTemperatureSource : public TemperatureDataSource {
        // Future: HTTP/MQTT remote data fetching
    };
}
```

**Rationale**:
- Simple interface follows clarification answer A: `float getTemperature(int x, int y)`
- Factory pattern enables runtime data source switching
- Minimal interface reduces future integration complexity
- Synchronous design matches current simulator patterns

**Alternatives considered**:
- Async callback interface: Rejected as unnecessary for proof-of-concept
- Event-driven interface: Rejected due to complexity violation

---

## Implementation Architecture Decisions

### RPC Command Flow
1. MQTT message received on `v1/devices/me/rpc/request/+`
2. `ThermalRPCHandler` parses JSON and validates parameters  
3. `ThermalSpotManager` executes command via `MeasurementSpot` operations
4. Response sent to `v1/devices/me/rpc/response/{request_id}`

### Component Responsibilities
- **ThermalRPCHandler**: MQTT message parsing, response formatting, error handling
- **ThermalSpotManager**: Spot lifecycle management, persistence, validation  
- **MeasurementSpot**: Individual spot state, temperature reading, JSON serialization
- **TemperatureDataSource**: Modular temperature calculation interface

### Error Handling Strategy
- Constitution-compliant: Exceptions for programming errors, error codes for recoverable failures
- RPC errors return JSON error responses (not exceptions)
- File corruption handled gracefully (start with empty spots)
- Invalid coordinates rejected with descriptive error messages

### Threading Model
- Single-threaded design with async MQTT callbacks (constitution requirement)
- Sequential RPC command processing (clarification: queue commands)
- No dynamic allocation during temperature reading (constitution requirement)

## Dependencies and Integration Points

### Existing Components to Extend
- `src/thermal/measurement_spot.cpp`: Add RPC-specific methods
- `src/thingsboard/paho_device.cpp`: Add RPC subscription and response handling
- `CMakeLists.txt`: Add new thermal/rpc modules to build

### New Components to Create
- `src/thermal/spot_manager.cpp`: Central spot coordination
- `src/thermal/temperature_source.cpp`: Modular temperature interface
- `src/thingsboard/rpc/thermal_handler.cpp`: RPC command processing
- `config/thermal_spots.json`: Runtime spot persistence

### Testing Strategy
- Unit tests: Extend existing test framework for new components
- Integration tests: Add RPC command flow testing with MQTT mocks
- Validation: Reuse existing `MeasurementSpot` and `TemperatureReading` test patterns

## Constitution Compliance Verification

✅ **C++17 Standards**: All proposed components use modern C++ patterns  
✅ **MQTT Library**: Extends existing Eclipse Paho integration  
✅ **Test Coverage**: Leverages existing Google Test infrastructure  
✅ **Connection Resilience**: RPC timeouts and error handling included  
✅ **Simplicity**: Minimal new abstractions, reuses existing components  
✅ **Memory Management**: Smart pointers and RAII patterns maintained  
✅ **Error Handling**: Consistent exception/error code separation  

## Next Steps for Phase 1

1. **Data Model Design**: Extract entities from feature spec for formal data model
2. **API Contracts**: Define RPC command/response schemas based on control.md
3. **Agent Context Update**: Add thermal RPC technology to agent configuration
4. **Quick Start Guide**: Document RPC command testing procedures

All NEEDS CLARIFICATION items from Technical Context have been resolved with concrete implementation decisions.