<!--
Sync Impact Report:
- Version change: none → 1.0.0 (initial constitution)
- Added principles: Modern C++ Standards, MQTT Library Integration, Test-First Development, Connection Resilience, Proof of Concept Simplicity
- Added sections: Technical Constraints, Development Workflow
- Templates requiring updates: ✅ constitution created (initial version)
- Follow-up TODOs: none
-->

# ThingsBoard MQTT Client Constitution

## Core Principles

### I. Modern C++ Standards
Every component MUST adhere to C++17 standards and best practices. All code MUST compile without warnings using `-Wall -Wextra -Werror`. Smart pointers (unique_ptr, shared_ptr) MUST be used for memory management. Raw pointers are prohibited except for non-owning references. RAII principles MUST be applied consistently for resource management.

**Rationale**: C++17 provides essential features like structured bindings, optional, and improved type deduction that enhance code safety and readability while maintaining performance.

### II. MQTT Library Integration (NON-NEGOTIABLE)
Eclipse Paho MQTT C++ library MUST be the exclusive MQTT implementation. All MQTT operations MUST go through Paho's async client interface. Direct socket programming or alternative MQTT libraries are forbidden. ThingsBoard-specific message formats and device protocols MUST be abstracted into separate classes.
The Paho library should be built as part of the application and should be linked statically. Place the library in directory 'paho'.

**Rationale**: Eclipse Paho provides mature, tested MQTT functionality. Standardizing on this library ensures reliability and reduces development time for a proof of concept.

### III. Test-First Development
Unit tests MUST be written before implementation using Google Test framework. Every public method and class MUST have corresponding tests. Test coverage below 80% blocks merge. Integration tests MUST verify MQTT broker connectivity and message flow to ThingsBoard.

**Rationale**: Given the network-dependent nature of MQTT communication, comprehensive testing prevents integration issues and ensures reliable broker communication.

### IV. Connection Resilience
MQTT connection handling MUST implement automatic reconnection with exponential backoff. Network failures MUST be handled gracefully without data loss. Connection state MUST be observable and logged. Message persistence during disconnection is REQUIRED for critical telemetry data.

**Rationale**: IoT applications require robust network handling. Poor connection management renders the proof of concept unusable in real-world scenarios.

### V. Proof of Concept Simplicity
Features MUST be minimal and focused on core MQTT functionality. No unnecessary abstractions or frameworks beyond essential requirements. Configuration MUST be simple (JSON file or environment variables). CLI interface MUST provide clear, actionable feedback.

**Rationale**: As a proof of concept, complexity must be minimized to demonstrate feasibility quickly while maintaining code quality.

## Technical Constraints

**Language**: C++17 (no newer standards to ensure broad compatibility)
**MQTT Library**: Eclipse Paho MQTT C++ (paho-mqtt-cpp)
**Build System**: CMake 3.16+ with clear dependency management
**Testing**: Google Test (gtest) for unit and integration testing
**Broker**: ThingsBoard Community Edition or Cloud
**Platforms**: Linux (primary), macOS (secondary), Windows (if time permits)
**Dependencies**: Minimal external dependencies beyond Paho MQTT and Google Test
**Memory**: No dynamic allocation during message processing (pre-allocated buffers)
**Threading**: Single-threaded design with async callbacks to avoid concurrency complexity

## Development Workflow

**Branching**: Feature branches with descriptive names (feature/mqtt-connection, fix/reconnection-logic)
**Code Review**: All changes require review before merge, focusing on C++17 compliance and test coverage
**Continuous Integration**: Automated builds and tests on Linux and macOS
**Documentation**: README with clear build and usage instructions, inline code documentation for public APIs
**Commit Messages**: Conventional commits format (feat:, fix:, test:, docs:)
**Error Handling**: Exceptions for programming errors, error codes for recoverable failures
**Logging**: Structured logging with configurable levels (DEBUG, INFO, WARN, ERROR)

## Governance

This constitution supersedes all other development practices and guidelines. All pull requests MUST demonstrate compliance with core principles before merge approval. Complexity must be explicitly justified with clear rationale documented in code comments. Constitution amendments require unanimous team approval and migration plan for existing code.

All code reviews MUST verify: C++17 compliance, test coverage, MQTT error handling, and adherence to simplicity principle.

**Version**: 1.0.0 | **Ratified**: 2025-10-23 | **Last Amended**: 2025-10-23
