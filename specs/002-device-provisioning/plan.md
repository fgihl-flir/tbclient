# Implementation Plan: Device Provisioning

**Branch**: `002-device-provisioning` | **Date**: 2025-10-23 | **Spec**: [Device Provisioning Spec](./spec.md)
**Input**: Feature specification from `/specs/002-device-provisioning/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/commands/plan.md` for the execution workflow.

## Summary

Add automatic device provisioning capability to the existing thermal camera MQTT client. When `provision.txt` file is detected at startup, the system will use credentials from `provision.json` to register a new device with ThingsBoard via MQTT-based provisioning, generate a unique device name using pattern `thermal-camera-XXXXXX`, and update `thermal_config.json` with the new credentials. Includes retry logic with exponential backoff and versioned configuration backups for safety.

## Technical Context

**Language/Version**: C++17 (required by constitution)
**Primary Dependencies**: Eclipse Paho MQTT C++ (paho-mqtt-cpp) statically linked, Google Test (gtest), nlohmann/json for JSON parsing
**Storage**: JSON configuration files (`thermal_config.json`, `provision.json`), versioned backup files with timestamps
**Testing**: Google Test for unit and integration testing (constitution requirement)
**Target Platform**: Linux (primary), macOS (secondary), Windows (if time permits)
**Project Type**: Extension to existing thermal camera MQTT client application
**Performance Goals**: Provisioning completion within 30 seconds, 99% success rate for valid credentials
**Constraints**: Builds on existing codebase, MQTT-based provisioning only, no HTTP API usage
**Scale/Scope**: Single device provisioning per execution, supports 100+ concurrent device registrations
**Integration Points**: ThingsBoard MQTT provisioning protocol, existing thermal camera client configuration system
**Unknown Areas Requiring Research**: 
- NEEDS CLARIFICATION: ThingsBoard MQTT provisioning message format and protocol flow
- NEEDS CLARIFICATION: Specific MQTT topics and message structure for device registration
- NEEDS CLARIFICATION: Response parsing for device credentials and access tokens
- NEEDS CLARIFICATION: Error codes and failure modes in ThingsBoard provisioning responses

## Constitution Check (Post-Design Re-evaluation)

*GATE: Re-checked after Phase 1 design completion.*

- [x] **C++17 Standards**: Design uses standard C++17 features (smart pointers, optional, structured bindings), compiles with strict warnings
- [x] **MQTT Library**: Provisioning extends existing Eclipse Paho MQTT C++ usage, no additional MQTT libraries introduced
- [x] **Test Coverage**: Comprehensive test plan includes unit tests for all provisioning components, integration tests for end-to-end flow
- [x] **Connection Resilience**: Leverages existing MQTT reconnection, adds provisioning-specific retry with exponential backoff (1s, 2s, 4s)
- [x] **Simplicity**: Minimal extension to existing architecture, reuses JSON parsing patterns, no new frameworks or complex abstractions
- [x] **Memory Management**: Data model uses std::unique_ptr and std::shared_ptr, RAII for resource management, no raw pointer ownership
- [x] **Error Handling**: Clear separation maintained - exceptions for programming errors, error codes for network/recoverable failures

✅ **FINAL GATE PASSED**: Post-design constitution check confirms no violations. Architecture maintains compliance with all core principles.

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

```text
# Existing ThingsBoard Thermal Camera MQTT Client - Device Provisioning Extension
src/
├── mqtt/                # Existing MQTT client wrapper and connection management
├── thingsboard/         # Existing ThingsBoard protocol + NEW: provisioning module
│   └── provisioning.cpp # NEW: Device provisioning implementation
├── config/              # Existing configuration management + NEW: provisioning config
│   └── provisioning.cpp # NEW: Provisioning configuration parser
├── utils/               # Existing utility classes + NEW: file management utilities
│   └── file_utils.cpp   # NEW: Backup and file handling utilities
└── main.cpp             # Existing CLI entry point + NEW: provisioning detection

tests/
├── unit/                # Existing unit tests + NEW: provisioning unit tests
│   └── test_provisioning.cpp # NEW: Provisioning component tests
├── integration/         # Existing integration tests + NEW: provisioning integration tests
│   └── test_provisioning_integration.cpp # NEW: End-to-end provisioning tests
└── mocks/               # Existing mock objects + NEW: provisioning mocks

include/                 # Existing headers + NEW: provisioning headers
├── mqtt/                # Existing MQTT headers
├── thingsboard/         # Existing ThingsBoard headers + provisioning.h
├── config/              # Existing config headers + provisioning.h
└── utils/               # Existing utils + file_utils.h

# Configuration Files (no changes to existing structure)
thermal_config.json      # Existing - will be updated with new device credentials
provision.json           # NEW: Provisioning credentials (user-provided)
provision.txt            # NEW: Trigger file for provisioning mode
```

**Structure Decision**: Extends existing thermal camera MQTT client architecture with minimal additions. Provisioning functionality integrated into existing `thingsboard/` module structure. Configuration management extended to handle provisioning files. File utilities added for safe configuration updates and backup management. No architectural changes to maintain constitution compliance and code simplicity.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |
