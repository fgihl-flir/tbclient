# MQTT Message Contracts

**Feature**: Thermal Camera MQTT Client  
**Date**: 2025-10-23  
**Protocol**: MQTT over TCP/TLS

## ThingsBoard MQTT Integration

### Connection Parameters

**Broker**: ThingsBoard MQTT Broker  
**Protocol**: MQTT v3.1.1  
**Port**: 1883 (non-SSL) or 8883 (SSL)  
**Authentication**: Access Token  

### Authentication Contract

```
Username: {device_access_token}
Password: (empty string)
Client ID: {configurable_device_id}
```

**Example**:
```
Username: "A1_TEST_TOKEN"
Password: ""
Client ID: "thermal_camera_001"
```

### Topic Structure

#### Telemetry Publishing

**Topic**: `v1/devices/me/telemetry`  
**QoS**: 1 (At least once delivery)  
**Retain**: false  
**Direction**: Client → ThingsBoard  

#### Attributes (Future Extension)

**Topic**: `v1/devices/me/attributes`  
**QoS**: 1  
**Retain**: false  
**Direction**: Client → ThingsBoard  

## Message Formats

### Single Spot Telemetry

**Topic**: `v1/devices/me/telemetry`

**Payload Schema**:
```json
{
  "spot": integer,
  "temperature": number
}
```

**Example**:
```json
{
  "spot": 1,
  "temperature": 85.7
}
```

**Field Specifications**:
- `spot`: Integer spot identifier (1-5, maximum 5 spots supported)
- `temperature`: Temperature in Celsius (-100.0 to 500.0, validated range)

### Multi-Spot Telemetry (Individual Messages)

**Topic**: `v1/devices/me/telemetry`

**Implementation**: Each measurement spot sends a separate individual message

**Example Sequence**:
```json
{"spot": 1, "temperature": 85.7}
{"spot": 2, "temperature": 156.2}
{"spot": 3, "temperature": 22.1}
```

**Constraints**:
- Maximum 5 measurement spots supported
- Each spot sends individual JSON message
- No batching or array format used

### Extended Telemetry (Optional)

**Topic**: `v1/devices/me/telemetry`

**Payload Schema**:
```json
{
  "spot": integer,
  "temperature": number,
  "quality": string,
  "timestamp": string
}
```

**Example**:
```json
{
  "spot": 1,
  "temperature": 85.7,
  "quality": "GOOD",
  "timestamp": "2025-10-23T10:30:00.000Z"
}
```

**Field Specifications**:
- `quality`: One of "GOOD", "POOR", "INVALID", "ERROR"
- `timestamp`: ISO 8601 format with milliseconds

## MQTT Client Behavior

### Connection Lifecycle

1. **Initial Connection**
   ```
   CONNECT → CONNACK
   ```

2. **Telemetry Transmission**
   ```
   PUBLISH (v1/devices/me/telemetry) → PUBACK
   ```

3. **Keep Alive**
   ```
   PINGREQ → PINGRESP (every keep_alive_seconds)
   ```

4. **Disconnect**
   ```
   DISCONNECT
   ```

### Error Handling

#### Authentication Failure
- **Response**: CONNACK with return code 4 (Bad User Name or Password)
- **Action**: Log error, stop connection attempts, require manual restart
- **Recovery**: Requires configuration update and manual restart

#### Network Failure
- **Response**: Connection timeout or socket error
- **Action**: Implement exponential backoff retry, discard any queued telemetry data
- **Recovery**: Automatic reconnection with fresh data

#### Message Delivery Failure
- **Response**: PUBACK timeout or error
- **Action**: Retry message up to configured attempts
- **Recovery**: Log failure, continue with next telemetry cycle

### Retry Strategy

**Exponential Backoff Parameters**:
- Initial delay: 1 second
- Maximum delay: 60 seconds
- Backoff multiplier: 2.0
- Maximum attempts: 10

**Retry Logic**:
```
delay = min(initial_delay * (multiplier ^ attempt), max_delay)
```

## Configuration Contract

### JSON Configuration Schema

```json
{
  "thingsboard": {
    "host": "string",
    "port": "integer",
    "access_token": "string",
    "device_id": "string",
    "use_ssl": "boolean",
    "keep_alive_seconds": "integer",
    "qos_level": "integer"
  },
  "telemetry": {
    "interval_seconds": "integer",
    "batch_transmission": "boolean",
    "retry_attempts": "integer",
    "retry_delay_ms": "integer",
    "measurement_spots": [
      {
        "id": "integer",
        "name": "string",
        "x": "integer",
        "y": "integer",
        "min_temp": "number",
        "max_temp": "number",
        "noise_factor": "number",
        "enabled": "boolean"
      }
    ]
  },
  "logging": {
    "level": "string",
    "console": "boolean"
  }
}
```

### Configuration Validation Rules

**ThingsBoard Section**:
- `host`: Non-empty string, valid hostname or IP
- `port`: Integer 1-65535
- `access_token`: Non-empty string, alphanumeric and special chars
- `device_id`: Non-empty string, alphanumeric and hyphens only
- `use_ssl`: Boolean
- `keep_alive_seconds`: Integer 10-300
- `qos_level`: Integer 0, 1, or 2

**Telemetry Section**:
- `interval_seconds`: Integer 1-3600
- `batch_transmission`: Boolean
- `retry_attempts`: Integer 0-10
- `retry_delay_ms`: Integer 100-10000
- `measurement_spots`: Array with 1-5 elements (maximum 5 spots)

**Measurement Spot Validation**:
- `id`: Unique integer 1-5 within device (maximum 5 spots)
- `name`: Non-empty string, max 50 characters
- `x`, `y`: Non-negative integers
- `min_temp`: Number -100.0 to 500.0
- `max_temp`: Number > min_temp, -100.0 to 500.0
- `noise_factor`: Number 0.0-1.0
- `enabled`: Boolean

## Error Response Contracts

### MQTT Error Codes

| Code | Meaning | Client Action |
|------|---------|---------------|
| 0 | Connection Accepted | Continue normal operation |
| 1 | Unacceptable protocol version | Log error, do not retry |
| 2 | Identifier rejected | Log error, change client ID |
| 3 | Server unavailable | Retry with backoff |
| 4 | Bad username or password | Log error, do not retry |
| 5 | Not authorized | Log error, check token |

### Application Error Codes

| Code | Category | Description | Recovery Action |
|------|----------|-------------|-----------------|
| 1000 | Configuration | Invalid JSON format | Fix configuration file |
| 1001 | Configuration | Missing required field | Add missing field |
| 1002 | Configuration | Invalid field value | Correct field value |
| 2000 | Connection | Network unreachable | Check network, retry |
| 2001 | Connection | Authentication failed | Check access token |
| 2002 | Connection | Broker unavailable | Check broker status, retry |
| 3000 | Telemetry | Invalid temperature reading | Skip reading, log warning, continue with other spots |
| 3001 | Telemetry | Message too large | Reduce measurements per message (N/A for individual messages) |
| 3002 | Telemetry | Transmission timeout | Retry with backoff |

## Message Flow Examples

### Successful Telemetry Flow

```
1. Client → Broker: CONNECT (with access token)
2. Broker → Client: CONNACK (success)
3. Client → Broker: PUBLISH (telemetry data)
4. Broker → Client: PUBACK (acknowledged)
5. [Wait 15 seconds]
6. Repeat step 3-5
```

### Reconnection Flow

```
1. Client detects connection loss
2. Client discards any queued telemetry data
3. Client waits 1 second (initial backoff)
4. Client → Broker: CONNECT
5. Broker → Client: CONNACK (success)
6. Client resumes telemetry transmission with fresh readings
```

### Error Recovery Flow

```
1. Client → Broker: PUBLISH (telemetry)
2. [Timeout - no PUBACK received]
3. Client retries PUBLISH (same message)
4. Broker → Client: PUBACK (acknowledged)
5. Client continues normal operation
```

## Test Scenarios

### Happy Path Tests
1. Successful connection with valid token
2. Regular telemetry transmission every 15 seconds (individual messages per spot)
3. Graceful shutdown with proper DISCONNECT

### Error Handling Tests
1. Invalid access token → authentication failure, stop retrying, require manual restart
2. Network disconnection → automatic reconnection, discard queued data
3. Broker unavailable → retry with exponential backoff
4. Invalid temperature reading → skip reading outside -100°C to 500°C range, log warning

### Configuration Tests
1. Valid configuration file → successful startup
2. Missing configuration file → error with helpful message
3. Invalid JSON → parse error with line number
4. Invalid field values → validation error with field name
5. More than 5 measurement spots → validation error