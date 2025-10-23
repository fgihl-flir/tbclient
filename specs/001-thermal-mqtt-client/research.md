# Research: Thermal Camera MQTT Client

**Feature**: Thermal Camera MQTT Client  
**Phase**: 0 (Research & Technology Decisions)  
**Date**: 2025-10-23

## Research Tasks Completed

### 1. Eclipse Paho MQTT C++ Static Linking

**Decision**: Use Eclipse Paho MQTT C++ with static linking and CMake integration

**Rationale**: 
- Static linking eliminates runtime dependencies and simplifies deployment
- CMake ExternalProject_Add can build Paho as part of main build process
- Reduces complexity for proof of concept deployment
- Aligns with constitution requirement for minimal external dependencies

**Implementation Approach**:
- Place Paho source in dedicated `paho/` directory
- Use CMake ExternalProject_Add or FetchContent to build Paho statically
- Link against libpaho-mqttpp3-static and libpaho-mqtt3a-static
- Include SSL support for secure ThingsBoard connections

**Alternatives Considered**:
- System package installation: Rejected due to deployment complexity
- Dynamic linking: Rejected due to runtime dependency requirements
- Alternative MQTT libraries: Rejected due to constitution mandate

### 2. ThingsBoard Access Token Authentication

**Decision**: Implement MQTT authentication using ThingsBoard access tokens

**Rationale**:
- ThingsBoard uses access tokens as username in MQTT authentication
- Password field should be empty or contain the token (depends on TB version)
- Simplest authentication method for proof of concept
- Well-documented and widely used approach

**Implementation Details**:
- MQTT username: device access token
- MQTT password: empty string (most common) or token (fallback)
- MQTT client ID: configurable device identifier
- Topic format: `v1/devices/me/telemetry` for telemetry publishing

**Alternatives Considered**:
- X.509 certificates: Rejected due to complexity for PoC
- OAuth2: Not supported by ThingsBoard MQTT interface
- Basic auth with separate credentials: Not standard ThingsBoard approach

### 3. JSON Configuration Management

**Decision**: Use nlohmann/json library for configuration parsing

**Rationale**:
- Header-only library aligns with minimal dependency principle
- Excellent C++17 integration with modern syntax
- Robust error handling and validation capabilities
- Widely adopted in C++ community

**Configuration Structure**:
```json
{
  "thingsboard": {
    "host": "thingsboard.cloud",
    "port": 1883,
    "access_token": "your_device_token",
    "device_id": "thermal_camera_001"
  },
  "telemetry": {
    "interval_seconds": 15,
    "measurement_spots": [
      {"id": 1, "name": "engine", "x": 100, "y": 150},
      {"id": 2, "name": "exhaust", "x": 200, "y": 300}
    ]
  },
  "logging": {
    "level": "INFO",
    "console": true
  }
}
```

**Alternatives Considered**:
- Environment variables only: Rejected due to complexity with nested config
- INI files: Rejected due to limited structure support
- YAML: Rejected due to additional dependency requirement

### 4. Thermal Camera Simulation Strategy

**Decision**: Implement simple thermal spot simulation with configurable temperature ranges

**Rationale**:
- Focus on MQTT functionality rather than complex thermal modeling
- Allows testing of multi-spot scenarios without hardware dependency
- Enables realistic temperature variation patterns for demonstration
- Simplified approach aligns with proof of concept goals

**Simulation Features**:
- Configurable temperature ranges per measurement spot
- Simple noise generation for realistic variation
- Time-based temperature changes (optional heating/cooling cycles)
- Validation of temperature readings before transmission

**Temperature Ranges by Spot Type**:
- Engine spots: 60-120°C with moderate variation
- Exhaust spots: 100-200°C with higher variation  
- Ambient spots: 15-35°C with low variation
- Custom spots: User-configurable ranges

### 5. Message Format and Telemetry Structure

**Decision**: Use specified JSON format with separate messages for each measurement spot

**Rationale**:
- Meets requirement for `{"spot": x, "temperature": yy}` format
- Clarified to send individual messages per spot rather than batched arrays
- Compatible with ThingsBoard telemetry processing
- Simple parsing and generation with nlohmann/json
- Better granular delivery tracking per spot

**Single Spot Format**:
```json
{"spot": 1, "temperature": 85.7}
```

**Multi-Spot Approach**:
- Send separate individual JSON messages for each measurement spot
- No array batching required
- Maximum 5 measurement spots supported (clarified limit)
- Each message sent independently with proper error handling

### 6. Error Handling and Logging Strategy

**Decision**: Implement structured logging with spdlog and exception-based error handling

**Rationale**:
- spdlog provides high-performance, configurable logging
- Aligns with constitution requirement for structured logging
- Exception handling for programming errors, error codes for recoverable failures
- Clear distinction between fatal errors and operational warnings

**Logging Categories**:
- Connection events (connect, disconnect, reconnect attempts)
- Telemetry transmission (success, failure, timing)
- Configuration changes and validation
- Thermal reading generation and validation
- Error conditions and recovery attempts

**Error Handling Patterns**:
- Exceptions: Configuration parsing errors, invalid parameters
- Error codes: Network failures, MQTT broker unavailable, invalid readings
- Automatic retry with exponential backoff for recoverable failures

## Technology Stack Summary

| Component | Technology | Rationale |
|-----------|------------|-----------|
| MQTT Library | Eclipse Paho C++ (static) | Constitution mandated, mature, reliable |
| JSON Processing | nlohmann/json | Header-only, C++17 compatible, minimal deps |
| Logging | spdlog | High performance, configurable, industry standard |
| Testing | Google Test | Constitution requirement, well-integrated |
| Build System | CMake 3.16+ | Cross-platform, Paho integration support |
| Configuration | JSON files | Simple, structured, human-readable |
| Authentication | Access tokens | ThingsBoard standard, simple implementation |

## Implementation Priorities

1. **Phase 1a**: CMake setup with static Paho building
2. **Phase 1b**: Basic configuration management and JSON parsing (max 5 spots)
3. **Phase 1c**: MQTT client wrapper with ThingsBoard authentication and auth failure handling
4. **Phase 1d**: Simple thermal spot simulation with temperature validation (-100°C to 500°C)
5. **Phase 1e**: Individual message telemetry transmission with 15-second intervals
6. **Phase 1f**: Connection resilience with automatic reconnection (discard queued data)
7. **Phase 1g**: Multi-spot support (up to 5 spots) with separate message transmission
8. **Phase 1h**: Error handling, logging, and graceful shutdown

## Risks and Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Paho static linking complexity | High | Use well-documented CMake patterns, test on multiple platforms |
| ThingsBoard authentication changes | Medium | Implement configurable auth methods, document token requirements |
| Network reliability in testing | Medium | Implement robust mock MQTT broker for unit tests |
| Configuration complexity growth | Low | Keep JSON structure flat, validate early and often, limit to 5 spots max |
| Constitution conflict on message persistence | Medium | Justified violation documented - discard queued data for PoC simplicity |
