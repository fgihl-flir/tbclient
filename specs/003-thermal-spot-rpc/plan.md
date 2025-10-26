# Implementation Plan: Thermal Camera Spot Measurement RPC Control

**Branch**: `003-thermal-spot-rpc` | **Date**: 26 October 2025 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/003-thermal-spot-rpc/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/commands/plan.md` for the execution workflow.

## Summary

Primary requirement: Implement thermal camera spot measurement control via ThingsBoard RPC commands (create, move, delete, list spots) with extension of existing basic thermal simulator. Technical approach: Modular C++17 implementation using Eclipse Paho MQTT for RPC communication, JSON configuration for spot persistence, and coordinate-based temperature simulation with location-dependent base temperatures and ±0.5°C random variation.

## Technical Context

**Language/Version**: C++17 (required by constitution)
**Primary Dependencies**: Eclipse Paho MQTT C++ (paho-mqtt-cpp) statically linked, Google Test (gtest), nlohmann/json for JSON parsing
**Storage**: JSON configuration files for spot persistence (`thermal_spots.json`), in-memory spot state management
**Testing**: Google Test for unit and integration testing (constitution requirement)
**Target Platform**: Linux (primary), macOS (secondary), Windows (if time permits)
**Project Type**: Extension of existing C++ thermal camera simulator with RPC integration
**Performance Goals**: RPC command response within 2 seconds, spot temperature readings within 500ms
**Constraints**: No dynamic allocation during message processing, 80%+ test coverage required, maximum 5 concurrent spots
**Scale/Scope**: Proof of concept - single thermal camera device simulation with spot measurement functionality

**Thermal Simulation**: Location-dependent base temperature calculation using coordinate-based algorithm, ±0.5°C random variation per reading, temperature range -40°C to +150°C

**RPC Integration**: ThingsBoard server-side RPC using MQTT topics `v1/devices/me/rpc/request/+` (subscribe) and `v1/devices/me/rpc/response/{request_id}` (publish), 5000ms command timeout

**Data Model**: Spot entities with spotId (1-5), x,y coordinates (0-319, 0-239 for 320x240 resolution), current temperature, active status

**Integration Points**: 
- NEEDS CLARIFICATION: How does existing thermal simulator integrate with new RPC handlers?
- NEEDS CLARIFICATION: What is the current thermal simulator architecture and interface?
- NEEDS CLARIFICATION: How should coordinate-based temperature algorithm be implemented?
- NEEDS CLARIFICATION: What JSON schema should be used for spot persistence?
- NEEDS CLARIFICATION: How should modular temperature data source interface be designed for future remote integration?

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**POST-DESIGN EVALUATION (Phase 1 Complete)**:

- [x] **C++17 Standards**: All thermal camera RPC and simulator code compiles with `-Wall -Wextra -Werror` without warnings
  - ✅ Design uses existing C++17 patterns from `MeasurementSpot` and `TemperatureReading` classes
  - ✅ Smart pointers and RAII enforced in `TemperatureDataSource` interface design
  - ✅ Exception handling follows existing patterns (programming errors vs recoverable failures)

- [x] **MQTT Library**: Only Eclipse Paho MQTT C++ library used for ThingsBoard RPC communication  
  - ✅ RPC handlers extend existing `paho_device.cpp` MQTT infrastructure
  - ✅ No additional MQTT libraries required
  - ✅ Reuses established MQTT topic subscription/response patterns

- [x] **Test Coverage**: 80%+ test coverage for spot management, RPC handlers, and temperature simulation
  - ✅ Design extends existing Google Test framework and patterns
  - ✅ Unit tests planned for each new component (ThermalSpotManager, RPC handlers, TemperatureDataSource)
  - ✅ Integration tests reuse existing MQTT mock patterns

- [x] **Connection Resilience**: RPC command handling includes timeout management and graceful error responses
  - ✅ 5000ms timeout enforcement in RPC command processing
  - ✅ Graceful error responses with specific error codes
  - ✅ Sequential command processing prevents race conditions

- [x] **Simplicity**: Modular extension of existing simulator without unnecessary abstractions
  - ✅ Reuses existing `MeasurementSpot` and `TemperatureReading` infrastructure
  - ✅ Single new abstraction: `TemperatureDataSource` interface (justified for future extensibility)
  - ✅ No additional frameworks or complex patterns introduced

- [x] **Memory Management**: Smart pointers for spot objects, RAII for temperature data source interface
  - ✅ `std::unique_ptr` for `TemperatureDataSource` instances
  - ✅ RAII pattern in spot persistence file handling
  - ✅ No raw pointer ownership in new components

- [x] **Error Handling**: Clear error codes for RPC responses, exceptions only for programming errors
  - ✅ RPC errors return structured JSON responses (not exceptions)
  - ✅ File corruption handled gracefully (start with empty spots)
  - ✅ Programming errors (invalid coordinates) use exceptions consistently

**FINAL STATUS**: ✅ **PASS** - All constitution requirements satisfied. Design maintains proof-of-concept simplicity while enabling required functionality through minimal, well-justified extensions to existing architecture.

## Project Structure

### Documentation (this feature)

```text
specs/003-thermal-spot-rpc/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
# Thermal Camera RPC Extension Structure
src/
├── mqtt/                # Existing MQTT client wrapper and connection management
├── thingsboard/         # Existing ThingsBoard protocol and message handling
│   └── rpc/             # NEW: RPC command handlers for thermal spot operations
├── thermal/             # NEW: Thermal camera simulation and spot management
│   ├── simulator/       # Extended thermal camera simulator
│   ├── spots/           # Spot measurement management
│   └── temperature/     # Temperature data source interface and algorithms
├── config/              # Existing configuration file parsing and management
├── utils/               # Existing utility classes and helper functions
└── main.cpp             # Existing CLI entry point (to be extended)

tests/
├── unit/                # Existing unit tests + new thermal/RPC tests
│   ├── thermal/         # NEW: Unit tests for thermal simulation
│   └── thingsboard/rpc/ # NEW: Unit tests for RPC handlers
├── integration/         # Existing integration tests + thermal RPC tests
└── mocks/               # Existing mock objects + thermal simulator mocks

include/                 # Public header files
├── mqtt/                # Existing MQTT headers
├── thingsboard/         # Existing ThingsBoard headers
│   └── rpc/             # NEW: RPC handler interfaces
├── thermal/             # NEW: Thermal simulation interfaces
│   ├── simulator/
│   ├── spots/
│   └── temperature/
├── config/              # Existing configuration headers
└── utils/               # Existing utility headers

config/
├── thermal_config.json  # Existing thermal device configuration
└── thermal_spots.json   # NEW: Persistent spot storage (runtime generated)
```

**Structure Decision**: Extension of existing C++ project structure for thermal camera RPC functionality. New `thermal/` module contains spot management, temperature simulation, and data source interface. RPC handlers added to existing `thingsboard/` module. Maintains constitution compliance with modular design and clear separation of concerns.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |
