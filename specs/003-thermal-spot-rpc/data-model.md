# Data Model: Thermal Camera Spot Measurement RPC Control

**Date**: 26 October 2025  
**Source**: Feature specification functional requirements FR-001 through FR-022  
**Purpose**: Define data entities, relationships, and validation rules for thermal spot RPC control

## Core Entities

### 1. SpotMeasurement

**Purpose**: Represents a temperature measurement point with RPC control capabilities

**Fields**:
- `spotId`: string (1-5) - Unique identifier within thermal camera
- `x`: integer (0-319) - X coordinate on thermal image  
- `y`: integer (0-239) - Y coordinate on thermal image
- `currentTemperature`: float (-40.0 to +150.0) - Last measured temperature in Celsius
- `baseTemperature`: float (-40.0 to +150.0) - Coordinate-calculated base temperature
- `status`: enum (active, inactive, error) - Current operational state
- `createdAt`: timestamp - When spot was created via RPC
- `lastReading`: timestamp - When temperature was last measured

**Validation Rules**:
- spotId must be numeric string "1", "2", "3", "4", or "5"
- x coordinate must be 0 ≤ x ≤ 319 (320px width)
- y coordinate must be 0 ≤ y ≤ 239 (240px height) 
- currentTemperature within global range -40°C to +150°C
- baseTemperature calculated using coordinate-based algorithm
- Maximum 5 concurrent active spots (FR-014)
- Duplicate spotId creation prevented (FR-013)

**State Transitions**:
```
[created] → active (via createSpotMeasurement RPC)
active → active (via moveSpotMeasurement RPC) 
active → inactive (via deleteSpotMeasurement RPC)
active → error (temperature reading failure)
error → active (via error recovery)
```

**Relationships**:
- 1:N with TemperatureReading (spot can have multiple readings over time)
- 1:1 with TemperatureDataSource coordinate (for base temperature calculation)

---

### 2. RPCCommand  

**Purpose**: Represents incoming ThingsBoard RPC request with processing state

**Fields**:
- `requestId`: string - Unique identifier from ThingsBoard RPC topic
- `method`: enum (createSpotMeasurement, moveSpotMeasurement, deleteSpotMeasurement, listSpotMeasurements)
- `parameters`: object - Command-specific parameters (see RPC schemas)
- `receivedAt`: timestamp - When command was received
- `processedAt`: timestamp - When command processing completed
- `timeout`: integer (5000) - Command timeout in milliseconds  
- `status`: enum (pending, processing, completed, error, timeout)

**Validation Rules**:
- requestId must be non-empty string from MQTT topic
- method must be one of four supported RPC commands
- parameters must match method-specific schema (see contracts/)
- timeout must be positive integer ≤ 30000ms
- Sequential processing enforced (FR-016)

**State Transitions**:
```
[received] → pending → processing → completed
                   ↘ → error
                   ↘ → timeout
```

**Relationships**:
- 1:1 with RPCResponse (every command generates exactly one response)
- 1:0..N with SpotMeasurement (commands may operate on 0 to many spots)

---

### 3. RPCResponse

**Purpose**: Represents outgoing response to ThingsBoard RPC command

**Fields**:
- `requestId`: string - Matches incoming command requestId  
- `result`: enum (success, error) - High-level result status
- `data`: object - Success response data (method-specific)
- `error`: object - Error details with code and message
- `responseTime`: integer - Processing time in milliseconds
- `sentAt`: timestamp - When response was published to MQTT

**Validation Rules**:
- requestId must match originating RPCCommand
- result "success" requires data object, prohibits error object
- result "error" requires error object, prohibits data object  
- responseTime must be positive integer
- Response must be sent within command timeout period

**Schema Structure**:
```json
// Success response
{
  "result": "success",
  "data": { /* method-specific success data */ }
}

// Error response  
{
  "result": "error", 
  "error": {
    "code": "ERROR_CODE",
    "message": "Human-readable error description"
  }
}
```

**Relationships**:
- 1:1 with RPCCommand (response belongs to exactly one command)
- Published to MQTT topic `v1/devices/me/rpc/response/{requestId}`

---

### 4. TemperatureDataSource

**Purpose**: Modular interface for temperature calculation strategies

**Fields**:
- `sourceName`: string - Identifier for data source type
- `isReady`: boolean - Whether source can provide temperature data
- `lastError`: string - Most recent error message (if any)
- `coordinateAlgorithm`: enum (distance_based, grid_based, noise_only)

**Methods**:
- `getTemperature(x, y)`: float - Calculate temperature for coordinates
- `validateCoordinates(x, y)`: boolean - Check if coordinates are valid
- `getBaseTemperature(x, y)`: float - Get coordinate-based base temperature

**Validation Rules**:
- sourceName must be non-empty string
- coordinateAlgorithm must implement coordinate-based calculation (FR-022)
- getTemperature must return value within -40°C to +150°C range  
- Random variation must be ±0.5°C per reading (clarification)

**Implementations**:
- `CoordinateBasedTemperatureSource`: Distance-from-center algorithm
- `RemoteTemperatureSource`: Future HTTP/MQTT remote data integration

**Relationships**:
- 1:N with SpotMeasurement (one source serves all active spots)
- Interface enables future modular expansion (FR-019)

---

## Data Flow Architecture

### 1. RPC Command Processing Flow

```
MQTT Topic → RPCCommand → ThermalSpotManager → SpotMeasurement → RPCResponse → MQTT Topic
             ↓
         [validation]
             ↓
         [persistence via thermal_spots.json]
             ↓
         [temperature calculation via TemperatureDataSource]
```

### 2. Spot Persistence Flow

```
SpotMeasurement ↔ JSON File (thermal_spots.json)
                ↗
    [graceful corruption handling - start with empty spots]
                ↗  
    [version field for future schema migration]
```

### 3. Temperature Calculation Flow

```
SpotMeasurement.coordinates → TemperatureDataSource.getTemperature(x,y) → base + random(±0.5°C)
                            ↗
                [coordinate algorithm: distance from center]
                            ↗
                [configurable base temperature range]
```

## Validation Matrix

| Entity | Field | Validation Rule | Error Handling |
|--------|-------|----------------|----------------|
| SpotMeasurement | spotId | "1"-"5" numeric string | Return "INVALID_SPOT_ID" |
| SpotMeasurement | x,y | 0-319, 0-239 pixel bounds | Return "INVALID_COORDINATES" |  
| SpotMeasurement | temperature | -40.0 to +150.0 Celsius | Return "TEMPERATURE_OUT_OF_RANGE" |
| RPCCommand | method | createSpot\|moveSpot\|deleteSpot\|listSpots | Return "UNKNOWN_METHOD" |
| RPCCommand | timeout | 1-30000 milliseconds | Return "INVALID_TIMEOUT" |
| RPCResponse | requestId | non-empty string match | Log error, no response |

## JSON Persistence Schema

### thermal_spots.json Structure

```json
{
  "version": "1.0",
  "thermal_spots": [
    {
      "spotId": "1",
      "x": 160,
      "y": 120, 
      "currentTemperature": 25.3,
      "baseTemperature": 25.1,
      "status": "active",
      "createdAt": "2025-10-26T10:30:00Z",
      "lastReading": "2025-10-26T10:35:00Z"
    }
  ],
  "lastUpdated": "2025-10-26T10:35:00Z",
  "totalActiveSpots": 1
}
```

### Error Recovery Rules

- **File not found**: Start with empty spots configuration
- **JSON parse error**: Log warning, start with empty spots  
- **Invalid spot data**: Skip corrupted spots, load valid spots
- **Schema version mismatch**: Attempt compatibility migration

## Entity Relationships Diagram

```
    RPCCommand (1) ←→ (1) RPCResponse
         ↓
    ThermalSpotManager
         ↓
    SpotMeasurement (0..5) ←→ (1) TemperatureDataSource
         ↓
    thermal_spots.json (persistence)
```

## Success Criteria Mapping

- **SC-001** (2 second response): RPCCommand.timeout enforcement
- **SC-002** (100% valid command processing): Validation matrix implementation  
- **SC-003** (1 second move completion): Move command optimization
- **SC-006** (500ms temperature reading): TemperatureDataSource performance requirement
- **SC-007** (5 concurrent spots): SpotMeasurement.MAX_SPOTS = 5 enforcement

This data model provides the foundation for implementing thermal spot RPC control with proper validation, persistence, and modular temperature calculation as specified in the functional requirements.