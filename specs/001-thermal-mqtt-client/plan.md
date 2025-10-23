# Implementation Plan: Thermal Camera MQTT Client

**Branch**: `001-thermal-mqtt-client` | **Date**: 2025-10-23 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/001-thermal-mqtt-client/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/commands/plan.md` for the execution workflow.

## Summary

Build a proof of concept C++ MQTT client for thermal camera devices that connects to ThingsBoard and transmits temperature telemetry data every 15 seconds. The client will use Eclipse Paho MQTT C++ library with static linking, support up to 5 measurement spots sending individual JSON messages, provide simple configuration through JSON files, and include robust error handling with specific behaviors for authentication failures and invalid temperature readings.

## Technical Context

**Language/Version**: C++17 (required by constitution)
**Primary Dependencies**: Eclipse Paho MQTT C++ (paho-mqtt-cpp) statically linked, Google Test (gtest)
**Storage**: JSON configuration files for connection settings and measurement spots, in-memory message buffers
**Testing**: Google Test for unit and integration testing (constitution requirement)
**Target Platform**: Linux (primary), macOS (secondary), Windows (if time permits)
**Project Type**: Single C++ CLI application with thermal camera simulation capabilities
**Performance Goals**: 15-second telemetry intervals, sub-second reconnection, handle up to 5 measurement spots
**Constraints**: No dynamic allocation during message processing, 80%+ test coverage required, static Paho linking, maximum 5 measurement spots
**Scale/Scope**: Proof of concept - thermal camera device simulation with up to 5 spots, basic telemetry transmission to ThingsBoard
**Authentication**: ThingsBoard access token-based authentication with manual restart on auth failure
**Configuration**: Simple JSON configuration for ThingsBoard endpoint, credentials, and measurement spots
**Build System**: CMake with static Paho MQTT library integration in 'paho' directory
**Error Handling**: Skip invalid temperature readings (-100°C to 500°C range), discard queued data on disconnection, stop retrying on auth failure

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- [x] **C++17 Standards**: All code compiles with `-Wall -Wextra -Werror` without warnings - Design uses modern C++17 features with strict compiler settings
- [x] **MQTT Library**: Only Eclipse Paho MQTT C++ library used for MQTT operations - Research confirmed static linking approach, no alternative libraries
- [x] **Test Coverage**: 80%+ test coverage with Google Test framework - Test structure planned with unit, integration, and mock components
- [ ] **Connection Resilience**: Automatic reconnection with exponential backoff implemented but WITHOUT message persistence (see Complexity Tracking)
- [x] **Simplicity**: No unnecessary abstractions, minimal dependencies beyond Paho and GTest - Dependencies limited to nlohmann/json, spdlog, minimal external libs
- [x] **Memory Management**: Smart pointers used, no raw pointer ownership - Data model designed with RAII principles, smart pointer usage throughout
- [x] **Error Handling**: Clear separation between exceptions (programming errors) and error codes (recoverable failures) - Error handling strategy documented with specific patterns for each error type

**Post-Design Evaluation**: One constitution principle partially modified - see Complexity Tracking for justification.

## Project Structure

### Documentation (this feature)

```text
specs/[###-feature]/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)
<!--
  ACTION REQUIRED: Replace the placeholder tree below with the concrete layout
  for this feature. Delete unused options and expand the chosen structure with
  real paths (e.g., apps/admin, packages/something). The delivered plan must
  not include Option labels.
-->

```text
# Thermal Camera MQTT Client Project Structure
src/
├── thermal/             # Thermal camera simulation and spot management
├── mqtt/                # MQTT client wrapper and connection management
├── thingsboard/         # ThingsBoard-specific protocol and message handling
├── config/              # Configuration file parsing and management
├── utils/               # Utility classes and helper functions
└── main.cpp             # CLI entry point

tests/
├── unit/                # Unit tests for individual classes
├── integration/         # Integration tests with MQTT broker
└── mocks/               # Mock objects for testing

include/
├── thermal/             # Thermal camera and measurement spot headers
├── mqtt/                # MQTT client headers
├── thingsboard/         # ThingsBoard protocol headers
├── config/              # Configuration management headers
└── utils/               # Utility headers

paho/                    # Eclipse Paho MQTT C++ library (static build)
├── src/                 # Paho source code
├── include/             # Paho headers
└── CMakeLists.txt       # Paho build configuration

CMakeLists.txt           # Main build configuration
config.json              # Default configuration file
README.md                # Build and usage instructions
.gitignore               # Git ignore rules
```

**Structure Decision**: C++ single project structure selected for thermal camera proof of concept. Modular design with separate directories for thermal camera simulation, MQTT handling, ThingsBoard protocol, and configuration management. CMake-based build system with static Paho MQTT library integration. Clear separation between source code (`src/`), headers (`include/`), and tests (`tests/`) following C++ best practices. Dedicated `paho/` directory for statically linked Eclipse Paho MQTT C++ library. Maximum 5 measurement spots supported as clarified in specification.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| No message persistence during disconnection | Proof of concept simplicity and clear data consistency - clarified that discarding queued data is preferred over complexity of persistence | Message persistence would require disk I/O, file management, and recovery logic that adds significant complexity to a thermal camera proof of concept where fresh readings are more valuable than stale data |
