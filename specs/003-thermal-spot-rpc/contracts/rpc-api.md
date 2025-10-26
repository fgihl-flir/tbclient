# ThingsBoard RPC Contracts: Thermal Camera Spot Measurement Control

**Protocol**: ThingsBoard Server-side RPC via MQTT  
**Date**: 26 October 2025  
**Version**: 1.0  
**Source**: Feature specification FR-001 through FR-010, control.md research

## MQTT Topics

### Subscription (Device receives commands)
```
Topic: v1/devices/me/rpc/request/+
QoS: 1 (At least once delivery)
Retain: false
```

### Response (Device sends responses)  
```
Topic: v1/devices/me/rpc/response/{request_id}
QoS: 1 (At least once delivery)  
Retain: false
```

## RPC Command Structure

### Base Command Format
```json
{
  "method": "string",
  "params": { 
    /* method-specific parameters */
  },
  "timeout": 5000
}
```

**Fields**:
- `method`: Required string, one of supported RPC methods
- `params`: Required object, parameters specific to method
- `timeout`: Optional integer, default 5000ms, max 30000ms

## Command Specifications

### 1. Create Spot Measurement

**Purpose**: Create new temperature measurement spot at specified coordinates

#### Request Schema
```json
{
  "method": "createSpotMeasurement",
  "params": {
    "spotId": "1",
    "x": 160,
    "y": 120
  },
  "timeout": 5000
}
```

**Parameters**:
- `spotId`: string, required, values "1"|"2"|"3"|"4"|"5"
- `x`: integer, required, range 0-319 (thermal image width)
- `y`: integer, required, range 0-239 (thermal image height)

#### Success Response Schema
```json
{
  "result": "success",
  "data": {
    "spotId": "1",
    "coordinates": {
      "x": 160,
      "y": 120
    },
    "currentTemp": 25.3,
    "baseTemp": 25.1,
    "status": "active",
    "createdAt": "2025-10-26T10:30:00Z"
  }
}
```

#### Error Response Schemas
```json
// Spot already exists
{
  "result": "error",
  "error": {
    "code": "SPOT_ALREADY_EXISTS",
    "message": "Spot with ID '1' already exists"
  }
}

// Invalid coordinates
{
  "result": "error", 
  "error": {
    "code": "INVALID_COORDINATES",
    "message": "Coordinates (x=400, y=300) exceed image bounds (320x240)"
  }
}

// Maximum spots reached
{
  "result": "error",
  "error": {
    "code": "MAX_SPOTS_REACHED", 
    "message": "Cannot create spot: maximum 5 spots already active"
  }
}
```

---

### 2. Move Spot Measurement

**Purpose**: Relocate existing measurement spot to new coordinates

#### Request Schema
```json
{
  "method": "moveSpotMeasurement",
  "params": {
    "spotId": "1",
    "x": 180,
    "y": 140
  },
  "timeout": 5000
}
```

**Parameters**:
- `spotId`: string, required, values "1"|"2"|"3"|"4"|"5"  
- `x`: integer, required, range 0-319
- `y`: integer, required, range 0-239

#### Success Response Schema
```json
{
  "result": "success",
  "data": {
    "spotId": "1",
    "oldPosition": {
      "x": 160,
      "y": 120
    },
    "newPosition": {
      "x": 180, 
      "y": 140
    },
    "currentTemp": 26.1,
    "baseTemp": 26.0,
    "movedAt": "2025-10-26T10:35:00Z"
  }
}
```

#### Error Response Schemas
```json
// Spot not found
{
  "result": "error",
  "error": {
    "code": "SPOT_NOT_FOUND",
    "message": "Spot with ID '5' does not exist"
  }
}

// Invalid coordinates
{
  "result": "error",
  "error": {
    "code": "INVALID_COORDINATES", 
    "message": "Coordinates (x=350, y=280) exceed image bounds (320x240)"
  }
}
```

---

### 3. Delete Spot Measurement

**Purpose**: Remove measurement spot and stop temperature monitoring

#### Request Schema
```json
{
  "method": "deleteSpotMeasurement", 
  "params": {
    "spotId": "1"
  },
  "timeout": 5000
}
```

**Parameters**:
- `spotId`: string, required, values "1"|"2"|"3"|"4"|"5"

#### Success Response Schema
```json
{
  "result": "success",
  "data": {
    "spotId": "1",
    "status": "deleted",
    "deletedAt": "2025-10-26T10:40:00Z",
    "lastTemp": 25.8
  }
}
```

#### Error Response Schemas  
```json
// Spot not found
{
  "result": "error",
  "error": {
    "code": "SPOT_NOT_FOUND",
    "message": "Spot with ID '8' does not exist"
  }
}
```

---

### 4. List All Spots

**Purpose**: Retrieve information about all active measurement spots

#### Request Schema
```json
{
  "method": "listSpotMeasurements",
  "params": {},
  "timeout": 5000
}
```

**Parameters**: None (empty object)

#### Success Response Schema
```json
{
  "result": "success", 
  "data": {
    "spots": [
      {
        "spotId": "1",
        "coordinates": {
          "x": 180,
          "y": 140  
        },
        "currentTemp": 26.1,
        "baseTemp": 26.0,
        "status": "active",
        "createdAt": "2025-10-26T10:30:00Z",
        "lastReading": "2025-10-26T10:45:00Z"
      },
      {
        "spotId": "3", 
        "coordinates": {
          "x": 200,
          "y": 100
        },
        "currentTemp": 28.5,
        "baseTemp": 28.2, 
        "status": "active",
        "createdAt": "2025-10-26T10:35:00Z",
        "lastReading": "2025-10-26T10:45:00Z"
      }
    ],
    "totalSpots": 2,
    "maxSpots": 5,
    "queriedAt": "2025-10-26T10:45:00Z"
  }
}
```

#### Success Response (No Active Spots)
```json
{
  "result": "success",
  "data": {
    "spots": [],
    "totalSpots": 0, 
    "maxSpots": 5,
    "queriedAt": "2025-10-26T10:45:00Z"
  }
}
```

## Common Error Responses

### System-Level Errors

```json
// Unknown method
{
  "result": "error",
  "error": {
    "code": "UNKNOWN_METHOD",
    "message": "RPC method 'invalidMethod' is not supported"
  }
}

// Invalid JSON
{
  "result": "error", 
  "error": {
    "code": "INVALID_JSON",
    "message": "Request contains malformed JSON"
  }
}

// Missing parameters
{
  "result": "error",
  "error": {
    "code": "MISSING_PARAMETERS",
    "message": "Required parameter 'spotId' is missing"
  }
}

// Thermal camera busy
{
  "result": "error",
  "error": {
    "code": "CAMERA_BUSY", 
    "message": "Thermal camera is currently calibrating, try again later"
  }
}

// System error
{
  "result": "error",
  "error": {
    "code": "INTERNAL_ERROR",
    "message": "Temperature sensor communication failed"
  }
}
```

## Error Code Reference

| Code | Description | Affected Commands |
|------|-------------|-------------------|
| `SPOT_ALREADY_EXISTS` | SpotId already in use | createSpotMeasurement |
| `SPOT_NOT_FOUND` | SpotId does not exist | moveSpotMeasurement, deleteSpotMeasurement |
| `INVALID_COORDINATES` | x,y outside image bounds | createSpotMeasurement, moveSpotMeasurement |
| `MAX_SPOTS_REACHED` | All 5 spots already active | createSpotMeasurement |
| `UNKNOWN_METHOD` | RPC method not supported | All commands |
| `INVALID_JSON` | Malformed request JSON | All commands |
| `MISSING_PARAMETERS` | Required parameter absent | All commands |
| `CAMERA_BUSY` | Thermal camera unavailable | All commands |
| `INTERNAL_ERROR` | System-level failure | All commands |

## Validation Rules

### SpotId Validation
- Must be string type
- Must be exactly one character: "1", "2", "3", "4", or "5"
- Case sensitive
- No leading/trailing whitespace

### Coordinate Validation  
- Must be integer type
- X coordinate: 0 ≤ x ≤ 319 (320 pixel width)
- Y coordinate: 0 ≤ y ≤ 239 (240 pixel height)
- Negative coordinates rejected
- Floating point coordinates rejected

### Timeout Validation
- Must be positive integer
- Range: 1000ms ≤ timeout ≤ 30000ms
- Default: 5000ms if not specified

## Response Timing Requirements

| Operation | Target Response Time | Success Criteria |
|-----------|---------------------|-------------------|
| createSpotMeasurement | < 2 seconds | SC-001 |
| moveSpotMeasurement | < 1 second | SC-003 |  
| deleteSpotMeasurement | < 1 second | - |
| listSpotMeasurements | < 500ms | SC-006 |

## MQTT Communication Patterns

### Request-Response Flow
1. ThingsBoard publishes command to `v1/devices/me/rpc/request/{request_id}`
2. Device receives command, extracts `request_id` from topic
3. Device processes command and generates response
4. Device publishes response to `v1/devices/me/rpc/response/{request_id}`
5. ThingsBoard receives response and correlates with original request

### Topic Parsing
```cpp
// Extract request ID from topic
std::string topic = "v1/devices/me/rpc/request/12345";
std::string request_id = topic.substr(topic.find_last_of('/') + 1);
// request_id = "12345"

// Construct response topic  
std::string response_topic = "v1/devices/me/rpc/response/" + request_id;
// response_topic = "v1/devices/me/rpc/response/12345"
```

### Message Ordering
- Commands processed sequentially (FR-016)
- First-come-first-served queue
- No parallel command execution
- Response sent after command completion

## Testing Examples

### Test Command Sequence
```bash
# 1. Create first spot
mosquitto_pub -h demo.thingsboard.io -t "v1/devices/me/rpc/request/001" \
  -u "ACCESS_TOKEN" -m '{"method":"createSpotMeasurement","params":{"spotId":"1","x":160,"y":120}}'

# 2. Create second spot  
mosquitto_pub -h demo.thingsboard.io -t "v1/devices/me/rpc/request/002" \
  -u "ACCESS_TOKEN" -m '{"method":"createSpotMeasurement","params":{"spotId":"2","x":200,"y":100}}'

# 3. Move first spot
mosquitto_pub -h demo.thingsboard.io -t "v1/devices/me/rpc/request/003" \
  -u "ACCESS_TOKEN" -m '{"method":"moveSpotMeasurement","params":{"spotId":"1","x":180,"y":140}}'

# 4. List all spots
mosquitto_pub -h demo.thingsboard.io -t "v1/devices/me/rpc/request/004" \
  -u "ACCESS_TOKEN" -m '{"method":"listSpotMeasurements","params":{}}'

# 5. Delete second spot
mosquitto_pub -h demo.thingsboard.io -t "v1/devices/me/rpc/request/005" \
  -u "ACCESS_TOKEN" -m '{"method":"deleteSpotMeasurement","params":{"spotId":"2"}}'
```

### Expected Response Monitoring
```bash
# Subscribe to responses
mosquitto_sub -h demo.thingsboard.io -t "v1/devices/me/rpc/response/+" \
  -u "ACCESS_TOKEN"
```

This contract specification provides complete API definition for thermal camera spot measurement control via ThingsBoard RPC, enabling reliable implementation and testing of the thermal spot management system.