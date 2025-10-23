# Feature Specification: Thermal Camera MQTT Client

**Feature Branch**: `001-thermal-mqtt-client`  
**Created**: 2025-10-23  
**Status**: Draft  
**Input**: User description: "We are building a proof of concept mqtt client for a thermal camera. The end goal is for the mqtt client to be able to do auto provision of the device, continouslyt send telemetry of the measure spots, have an interface for thingsboard to create and change measure spots. Right we will concentrate om doing the basic framework, i.e. be able to build the paho library and our mqtt client. The client should connect thingsboard and send telemetry every 15s. the telentry should be in json format { spot:x, temperatur:yy}"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Basic MQTT Connection and Telemetry (Priority: P1)

A thermal camera device needs to establish a reliable connection to ThingsBoard and begin sending temperature measurements at regular intervals to demonstrate basic functionality.

**Why this priority**: This is the foundational capability required for any thermal monitoring system. Without reliable connectivity and basic telemetry transmission, no other features can function.

**Independent Test**: Can be fully tested by running the client, observing successful connection to ThingsBoard, and verifying temperature telemetry messages appear in the ThingsBoard device dashboard every 15 seconds.

**Acceptance Scenarios**:

1. **Given** the client is configured with valid ThingsBoard credentials, **When** the client starts, **Then** it successfully connects to the ThingsBoard MQTT broker
2. **Given** the client is connected to ThingsBoard, **When** 15 seconds elapses, **Then** a temperature telemetry message is sent in JSON format
3. **Given** the client is running, **When** network connectivity is temporarily lost, **Then** the client automatically reconnects and resumes sending telemetry

---

### User Story 2 - Multi-Spot Temperature Monitoring (Priority: P2)

The thermal camera system needs to monitor multiple measurement spots simultaneously and report temperature data for each spot to enable comprehensive thermal analysis.

**Why this priority**: Multi-spot monitoring is essential for practical thermal camera applications, allowing operators to track temperature variations across different areas of interest.

**Independent Test**: Can be tested by configuring multiple measurement spots, running the client, and verifying that telemetry data for each spot is transmitted with unique spot identifiers.

**Acceptance Scenarios**:

1. **Given** multiple measurement spots are configured, **When** telemetry is sent, **Then** each spot's temperature data is included with its unique identifier
2. **Given** a measurement spot is added during runtime, **When** the next telemetry cycle occurs, **Then** the new spot's data is included in transmission
3. **Given** temperature readings are available for all configured spots, **When** telemetry is sent, **Then** each spot's data is sent as a separate individual JSON message

---

### User Story 3 - Configuration Management (Priority: P3)

Operators need to configure MQTT connection parameters, measurement intervals, and spot definitions without rebuilding the application to adapt the system to different deployment environments.

**Why this priority**: Configuration flexibility is important for deployment across different environments and use cases, but can be implemented after core functionality is proven.

**Independent Test**: Can be tested by modifying configuration files, restarting the client, and verifying that new settings take effect without code changes.

**Acceptance Scenarios**:

1. **Given** a configuration file exists, **When** connection parameters are modified, **Then** the client uses the new settings on restart
2. **Given** the telemetry interval is changed in configuration, **When** the client runs, **Then** telemetry is sent at the new interval
3. **Given** measurement spots are defined in configuration, **When** the client starts, **Then** temperature monitoring begins for all configured spots

### Edge Cases

- What happens when ThingsBoard broker is temporarily unavailable? → Discard queued data and resume with fresh readings after reconnection
- How does the system handle invalid temperature readings from the thermal camera? → Skip invalid readings (outside -100°C to 500°C range) and log warnings
- What occurs when network latency causes message delivery delays?
- How does the client behave when ThingsBoard device credentials are invalid? → Log authentication error and stop connection attempts, require manual restart
- What happens if the thermal camera hardware becomes unresponsive?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST establish MQTT connection to ThingsBoard broker using device credentials and stop connection attempts on authentication failure, requiring manual restart
- **FR-002**: System MUST send temperature telemetry in JSON format {"spot": x, "temperature": yy} every 15 seconds
- **FR-003**: System MUST support monitoring multiple measurement spots with unique identifiers (maximum 5 spots) and send each spot's data as separate JSON messages
- **FR-004**: System MUST automatically reconnect to ThingsBoard when connection is lost and discard any queued telemetry data during disconnection
- **FR-005**: System MUST log connection status and telemetry transmission events
- **FR-006**: System MUST validate temperature readings before transmission and skip invalid readings (outside reasonable range -100°C to 500°C) with warning log, continuing with other spots
- **FR-007**: System MUST handle configuration through external files (JSON or similar format)
- **FR-008**: System MUST provide clear startup and runtime status messages
- **FR-009**: System MUST gracefully shut down when terminated
- **FR-010**: System MUST compile and run using Eclipse Paho MQTT C++ library

### Key Entities

- **Thermal Camera Device**: Represents the physical thermal imaging hardware that captures temperature measurements from defined spots
- **Measurement Spot**: A defined location or area within the thermal camera's field of view with unique identifier and coordinates
- **Temperature Reading**: Time-stamped temperature measurement from a specific spot, including spot identifier and temperature value
- **MQTT Client**: The software component responsible for managing ThingsBoard connectivity and message transmission
- **ThingsBoard Device**: The virtual representation of the thermal camera in the ThingsBoard platform for receiving and processing telemetry

## Success Criteria *(mandatory)*

## Clarifications

### Session 2025-10-23

- Q: For temperature reading validation (FR-006), what should happen when a measurement spot reports an obviously invalid temperature (e.g., -1000°C or 5000°C)? → A: Skip the invalid reading and log a warning, continue with other spots
- Q: When the client loses connection to ThingsBoard and has measurement data waiting to be sent, what should happen to that queued telemetry data? → A: Discard all queued data immediately when connection is lost
- Q: For User Story 2 acceptance scenario 3, should multiple measurement spots be sent as one JSON message with an array, or as separate individual messages? → A: Send separate individual messages for each spot
- Q: What should be the maximum number of measurement spots that the system needs to support simultaneously? → A: 5 spot is max
- Q: When ThingsBoard device credentials are invalid (authentication failure), should the client retry the connection or require manual intervention? → A: Log error and stop attempting connections, require manual restart

### Measurable Outcomes

- **SC-001**: Client successfully connects to ThingsBoard within 5 seconds of startup in 95% of attempts
- **SC-002**: Temperature telemetry is transmitted every 15 seconds (±1 second) with 99% consistency during normal operation
- **SC-003**: Client automatically recovers from network disconnections within 30 seconds
- **SC-004**: System can monitor and report temperature data for up to 5 different measurement spots simultaneously
- **SC-005**: Application starts successfully from configuration files without manual parameter entry
- **SC-006**: Telemetry messages are received and displayed correctly in ThingsBoard device dashboard
- **SC-007**: Client operates continuously for at least 24 hours without manual intervention or memory leaks
