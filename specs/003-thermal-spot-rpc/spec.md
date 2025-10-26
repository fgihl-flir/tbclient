# Feature Specification: Thermal Camera Spot Measurement RPC Control

**Feature Branch**: `003-thermal-spot-rpc`  
**Created**: 26 October 2025  
**Status**: Draft  
**Input**: User description: "We are adding control of the spot measure functions. We will implement create/delete/move commands. We will use thingsboard RPC commands. in the file control.md I already done some reasearch. We will need to handle these new command. 
The application already has a basic thermal camera measuring simulator. This need to be expanded to support creating(enable)/delete(disable)/move, of measuring spots. We will later expand the thermal backend more, so we can fetch temperatures from a remove source, so it important this the simlator is modular built."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Create Spot Measurement (Priority: P1)

A thermal camera operator needs to define a new measurement point on the thermal image to monitor temperature at a specific location. They send a command through ThingsBoard to create a spot measurement at specified coordinates.

**Why this priority**: This is the foundation capability - without creating spots, no temperature monitoring can occur. Delivers immediate value by enabling basic temperature measurement functionality.

**Independent Test**: Can be fully tested by sending a create command via ThingsBoard RPC and verifying the thermal camera acknowledges the new spot and starts reporting temperature data for that location.

**Acceptance Scenarios**:

1. **Given** a thermal camera is connected and operational, **When** operator sends createSpotMeasurement command with valid coordinates and spot ID "1", **Then** the camera creates the spot and responds with success confirmation and current temperature reading
2. **Given** a thermal camera already has spot "1" active, **When** operator tries to create spot "1" again, **Then** the camera responds with appropriate error indicating spot already exists
3. **Given** operator sends createSpotMeasurement command with invalid coordinates (outside image bounds), **When** the command is processed, **Then** the camera responds with coordinate validation error

---

### User Story 2 - Move Existing Spot Measurement (Priority: P2)

An operator needs to relocate an existing measurement spot to a different position on the thermal image when the area of interest shifts or they need to refine the measurement location.

**Why this priority**: Essential for operational flexibility but depends on spots already existing. Provides significant value for ongoing monitoring and measurement refinement.

**Independent Test**: Can be tested by first creating a spot, then sending a move command with new coordinates and verifying the spot reports temperature from the new location.

**Acceptance Scenarios**:

1. **Given** spot "1" exists at coordinates (160, 120), **When** operator sends moveSpotMeasurement command to coordinates (180, 140), **Then** the camera moves the spot and responds with old position, new position, and current temperature at new location
2. **Given** operator tries to move non-existent spot "5", **When** the move command is processed, **Then** the camera responds with "spot not found" error
3. **Given** operator tries to move spot "1" to invalid coordinates, **When** the command is processed, **Then** the camera responds with coordinate validation error

---

### User Story 3 - Delete Spot Measurement (Priority: P3)

An operator needs to remove measurement spots that are no longer needed to clean up the monitoring configuration and reduce system load.

**Why this priority**: Important for maintenance and configuration management but not critical for basic operation. Provides value for long-term system management.

**Independent Test**: Can be tested by creating a spot, deleting it, and verifying it no longer appears in spot listings and stops reporting temperature data.

**Acceptance Scenarios**:

1. **Given** spot "2" exists and is actively measuring, **When** operator sends deleteSpotMeasurement command for spot "2", **Then** the camera removes the spot and responds with deletion confirmation
2. **Given** operator tries to delete non-existent spot "8", **When** the delete command is processed, **Then** the camera responds with "spot not found" error

---

### User Story 4 - List All Active Spots (Priority: P2)

An operator needs to view all currently active measurement spots to understand the current monitoring configuration and verify spot status.

**Why this priority**: Essential for system visibility and management, especially when multiple spots are configured. Supports troubleshooting and configuration verification.

**Independent Test**: Can be tested by creating multiple spots and verifying the list command returns accurate information about all active spots including positions and current temperatures.

**Acceptance Scenarios**:

1. **Given** thermal camera has spots "1" and "3" active, **When** operator sends listSpotMeasurements command, **Then** the camera responds with array containing both spots with their IDs, coordinates, current temperatures, and status
2. **Given** thermal camera has no active spots, **When** operator sends listSpotMeasurements command, **Then** the camera responds with empty spots array and totalSpots count of 0

---

### User Story 5 - Expand Thermal Camera Simulator (Priority: P0)

A developer needs to extend the existing basic thermal camera measuring simulator to support dynamic spot management (create/enable, delete/disable, move) with a modular architecture that allows future expansion for remote temperature data sources.

**Why this priority**: This is foundational infrastructure work that enables all other user stories. The existing simulator must be enhanced before RPC commands can control spot measurements. Modular design ensures future extensibility for remote data sources.

**Independent Test**: Can be fully tested by verifying the simulator can dynamically enable/disable spots, move spots to new coordinates, persist spot configurations, and provide a clean interface for future temperature data source integration.

**Acceptance Scenarios**:

1. **Given** the existing basic thermal simulator is running, **When** developer calls enableSpot(spotId, x, y), **Then** the simulator creates a new active spot and begins generating temperature data for that location
2. **Given** an active spot exists at coordinates (100, 100), **When** developer calls moveSpot(spotId, newX, newY), **Then** the simulator updates the spot coordinates and continues temperature generation at the new location
3. **Given** multiple active spots exist, **When** developer calls disableSpot(spotId), **Then** the simulator removes the spot and stops generating temperature data for that location
4. **Given** the simulator architecture, **When** developer examines the code structure, **Then** temperature data source interface is clearly separated to allow future integration with remote temperature sources

---

### Edge Cases

- What happens when thermal camera is disconnected during RPC command processing?
- How does system handle simultaneous RPC commands for the same spot? (Commands queued sequentially, first-come-first-served)
- What occurs when thermal camera backend simulation is restarted while spots are active? (Spots persist across restarts via file storage)
- How does system respond to malformed JSON in RPC commands?
- What happens when maximum spot limit (5) is reached and operator tries to create another spot?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST process ThingsBoard RPC commands for createSpotMeasurement with parameters spotId (1-5), x coordinate (0-319), and y coordinate (0-239)
- **FR-002**: System MUST process ThingsBoard RPC commands for moveSpotMeasurement with parameters spotId and new x,y coordinates
- **FR-003**: System MUST process ThingsBoard RPC commands for deleteSpotMeasurement with parameter spotId
- **FR-004**: System MUST process ThingsBoard RPC commands for listSpotMeasurements with no parameters
- **FR-005**: System MUST validate coordinates are within thermal image bounds (320x240 resolution)
- **FR-006**: System MUST validate spotId is numeric string between "1" and "5"
- **FR-007**: System MUST respond to successful RPC commands with JSON containing result "success" and relevant data
- **FR-008**: System MUST respond to failed RPC commands with JSON containing result "error" and error details with code and message
- **FR-009**: System MUST subscribe to ThingsBoard RPC request topic "v1/devices/me/rpc/request/+"
- **FR-010**: System MUST publish RPC responses to "v1/devices/me/rpc/response/{request_id}"
- **FR-011**: Simulated thermal camera backend MUST maintain active spot measurements and simulate temperature readings for each spot
- **FR-012**: System MUST persist active spots to file and restore them on system restart
- **FR-013**: System MUST prevent creation of duplicate spots (same spotId)
- **FR-014**: System MUST limit maximum concurrent spots to 5
- **FR-015**: Each spot MUST report simulated temperature readings in range -40°C to +150°C with location-dependent base temperature and ±0.5°C random variation on each reading
- **FR-016**: System MUST queue simultaneous RPC commands and process them sequentially to prevent race conditions
- **FR-017**: Thermal camera simulator MUST extend existing basic simulator with modular architecture to support dynamic spot management
- **FR-018**: Thermal camera simulator MUST provide clean interface for enabling/disabling spots programmatically
- **FR-019**: Thermal camera simulator MUST implement modular temperature data source interface using simple callback: `float getTemperature(int x, int y)`
- **FR-020**: System MUST handle RPC command timeout of 5000ms as specified in control.md
- **FR-021**: System MUST persist spots to JSON file format and gracefully handle file corruption by starting with empty spots configuration
- **FR-022**: System MUST calculate location-dependent base temperatures using coordinate-based algorithm for realistic thermal simulation

### Key Entities

- **Spot Measurement**: Represents a temperature measurement point with spotId (1-5), x,y coordinates, current temperature reading, and active status
- **RPC Command**: Represents incoming ThingsBoard RPC request with method name, parameters, timeout, and unique request ID for response correlation
- **Thermal Camera Backend**: Modular simulated thermal imaging system that extends existing basic simulator to support dynamic spot management
- **Temperature Data Source Interface**: Modular interface design that separates temperature generation logic to enable future integration with remote temperature sources

## Success Criteria *(mandatory)*

## Clarifications

### Session 2025-10-26

- Q: How should temperature values change over time for active spots in the simulated thermal camera backend? → A: Location-dependent base temperature with ±0.5°C random variation on each reading
- Q: What happens to active spots when the thermal camera system restarts or reconnects to ThingsBoard? → A: Spots persist across restarts (saved to file)
- Q: How should the system handle simultaneous RPC commands for the same spot? → A: Queue commands sequentially (first-come-first-served)

### Session 2025-10-26 (Round 2)

- Q: What specific methods and abstraction should the temperature data source interface provide for modular architecture? → A: Simple callback interface: `float getTemperature(int x, int y)`
- Q: What file format should be used for spot persistence and how should corruption be handled? → A: JSON format with graceful degradation (start with empty spots if corrupted)
- Q: How should base temperatures be determined for each spot in the simulation? → A: Location-dependent base temperature using coordinate-based algorithm with ±0.5°C randomness on each temperature reading

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Operators can create new spot measurements in under 2 seconds from command sent to confirmation received
- **SC-002**: System successfully processes 100% of valid RPC commands without errors
- **SC-003**: Spot movement commands complete in under 1 second with accurate position updates
- **SC-004**: System maintains stable spot measurements for continuous operation over 24 hours
- **SC-005**: RPC command validation prevents 100% of invalid coordinate submissions (outside image bounds)
- **SC-006**: Thermal camera backend simulation provides temperature readings within 500ms of spot creation or movement
- **SC-007**: System supports concurrent management of all 5 available spots without performance degradation

## Assumptions

- ThingsBoard RPC infrastructure is already functional and accessible
- Thermal camera device is already provisioned and connected to ThingsBoard
- Basic thermal camera measuring simulator already exists and is operational
- Simulated thermal camera backend will generate location-dependent base temperatures with ±0.5°C random variation on each temperature reading
- MQTT connection between thermal camera and ThingsBoard is stable and reliable
- Coordinate system uses standard image conventions (0,0 at top-left, x increases rightward, y increases downward)
- Temperature simulation will use reasonable ambient temperature ranges unless specific heating/cooling patterns are simulated
- Existing simulator architecture can be extended with modular design patterns

## Dependencies

- Existing thermal camera MQTT client infrastructure
- ThingsBoard device provisioning and connectivity (already implemented)
- Existing basic thermal camera measuring simulator (to be extended)
- JSON parsing capabilities for RPC command processing
- MQTT publish/subscribe functionality for RPC communication
- Modular temperature data generation interface for future extensibility
