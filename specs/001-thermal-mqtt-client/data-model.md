# Data Model: Thermal Camera MQTT Client

**Feature**: Thermal Camera MQTT Client  
**Phase**: 1 (Design)  
**Date**: 2025-10-23

## Core Entities

### 1. Configuration

**Purpose**: Represents the complete application configuration loaded from JSON files

**Attributes**:
- `thingsboard_config`: ThingsBoardConfig object containing connection parameters
- `telemetry_config`: TelemetryConfig object containing measurement and timing settings
- `logging_config`: LoggingConfig object containing log level and output settings

**Validation Rules**:
- Configuration file must be valid JSON
- All required fields must be present
- Numeric values must be within reasonable ranges
- Access token must not be empty
- Telemetry interval must be between 1 and 3600 seconds

**State Transitions**:
- Unloaded → Loaded (via file parsing)
- Loaded → Validated (via validation check)
- Validated → Active (via application startup)

### 2. ThingsBoardConfig

**Purpose**: Contains all ThingsBoard-specific connection and authentication parameters

**Attributes**:
- `host`: string - ThingsBoard server hostname or IP address
- `port`: int - MQTT broker port (typically 1883 or 8883)
- `access_token`: string - Device access token for authentication
- `device_id`: string - Unique device identifier
- `use_ssl`: bool - Whether to use SSL/TLS encryption
- `keep_alive_seconds`: int - MQTT keep-alive interval
- `qos_level`: int - MQTT Quality of Service level (0, 1, or 2)

**Validation Rules**:
- Host must not be empty
- Port must be between 1 and 65535
- Access token must not be empty and contain valid characters
- Device ID must be unique and contain only alphanumeric characters and hyphens
- Keep alive must be between 10 and 300 seconds
- QoS level must be 0, 1, or 2

### 3. TelemetryConfig

**Purpose**: Defines telemetry transmission parameters and measurement spot configurations

**Attributes**:
- `interval_seconds`: int - Time between telemetry transmissions
- `measurement_spots`: vector<MeasurementSpot> - List of thermal measurement spots
- `batch_transmission`: bool - Whether to send all spots in one message or separately
- `retry_attempts`: int - Number of transmission retry attempts on failure
- `retry_delay_ms`: int - Delay between retry attempts

**Validation Rules**:
- Interval must be between 1 and 3600 seconds
- Must have at least one measurement spot
- Maximum 5 measurement spots per device (clarified limit)
- Retry attempts must be between 0 and 10
- Retry delay must be between 100 and 10000 milliseconds

### 4. MeasurementSpot

**Purpose**: Represents a single thermal measurement point with configuration and state

**Attributes**:
- `id`: int - Unique spot identifier within the device
- `name`: string - Human-readable spot name
- `x`: int - X coordinate in thermal image (pixels)
- `y`: int - Y coordinate in thermal image (pixels)
- `min_temp`: double - Minimum expected temperature (°C)
- `max_temp`: double - Maximum expected temperature (°C)
- `noise_factor`: double - Temperature variation noise factor (0.0-1.0)
- `enabled`: bool - Whether this spot is actively monitored

**Validation Rules**:
- ID must be unique within the device
- Name must not be empty and contain valid characters
- Coordinates must be non-negative
- Min temperature must be less than max temperature
- Temperature range must be between -100°C and 500°C (validation range)
- Noise factor must be between 0.0 and 1.0

**State Transitions**:
- Inactive → Active (when enabled = true)
- Active → Inactive (when enabled = false)
- Active → Reading (during temperature measurement)
- Reading → Active (after measurement completion)

### 5. TemperatureReading

**Purpose**: Represents a single temperature measurement from a specific spot at a point in time

**Attributes**:
- `spot_id`: int - ID of the measurement spot
- `temperature`: double - Temperature value in Celsius
- `timestamp`: time_point - When the measurement was taken
- `quality`: ReadingQuality enum - Measurement quality indicator
- `error_code`: optional<int> - Error code if measurement failed

**Validation Rules**:
- Spot ID must correspond to a valid configured spot
- Temperature must be within -100°C to 500°C validation range (invalid readings skipped with warning)
- Timestamp must not be in the future
- Quality must be one of: GOOD, POOR, INVALID, ERROR
- Error code required if quality is ERROR

**ReadingQuality Enum**:
- `GOOD`: Measurement within expected range and confidence
- `POOR`: Measurement outside normal range but possibly valid
- `INVALID`: Measurement clearly invalid (sensor error, out of range)
- `ERROR`: Measurement failed due to system error

### 6. MQTTClientState

**Purpose**: Tracks the current state and health of the MQTT client connection

**Attributes**:
- `connection_state`: ConnectionState enum - Current connection status
- `last_connect_time`: time_point - When connection was last established
- `last_message_time`: time_point - When last message was sent successfully
- `reconnect_attempts`: int - Number of reconnection attempts since last success
- `total_messages_sent`: int - Counter of successfully sent messages
- `total_errors`: int - Counter of transmission errors

**ConnectionState Enum**:
- `DISCONNECTED`: Not connected to broker
- `CONNECTING`: Connection attempt in progress
- `CONNECTED`: Successfully connected and authenticated
- `RECONNECTING`: Attempting to reconnect after failure
- `FAILED`: Connection permanently failed (requires manual intervention)

**State Transitions**:
- DISCONNECTED → CONNECTING (via connect attempt)
- CONNECTING → CONNECTED (via successful connection)
- CONNECTING → FAILED (via authentication failure - stop retrying, require manual restart)
- CONNECTED → DISCONNECTED (via network failure)
- CONNECTED → RECONNECTING (via connection loss with auto-retry, discard queued data)
- RECONNECTING → CONNECTED (via successful reconnection)
- RECONNECTING → FAILED (via exhausted retry attempts)

### 7. TelemetryMessage

**Purpose**: Represents a complete telemetry message ready for transmission to ThingsBoard

**Attributes**:
- `device_id`: string - Device identifier
- `timestamp`: time_point - Message creation time
- `measurements`: vector<TemperatureReading> - Temperature readings to transmit
- `message_id`: string - Unique message identifier for tracking
- `retry_count`: int - Number of times this message has been retried

**Validation Rules**:
- Device ID must match configured device
- Must contain at least one measurement
- Maximum 5 measurements per message (reflecting 5-spot limit)
- Message ID must be unique
- Retry count must not exceed configured maximum

**JSON Serialization Format** (Individual Messages):
```json
{
  "spot": 1,
  "temperature": 85.7
}
```

Note: Each measurement spot sends a separate individual JSON message. No batch/array format used as clarified in specification.

## Entity Relationships

```
Configuration
├── ThingsBoardConfig (1:1)
├── TelemetryConfig (1:1)
│   └── MeasurementSpot (1:many)
└── LoggingConfig (1:1)

MeasurementSpot
└── TemperatureReading (1:many over time)

MQTTClientState
└── TelemetryMessage (tracks transmission of)

TelemetryMessage
└── TemperatureReading (contains 1:many)
```

## Data Flow

1. **Startup**: Configuration loaded and validated
2. **Initialization**: MeasurementSpots created from configuration (max 5 spots)
3. **Runtime Loop**:
   - Generate TemperatureReadings for each enabled MeasurementSpot
   - Validate readings (skip invalid temperatures outside -100°C to 500°C)
   - Create individual TelemetryMessage for each valid reading
   - Transmit each message separately via MQTT client
   - Update MQTTClientState based on transmission result
   - On disconnection: discard any queued data, resume with fresh readings after reconnection
   - Wait for configured interval
   - Repeat

## Error Handling Data

### ConfigurationError
- `error_type`: enum (MISSING_FILE, INVALID_JSON, VALIDATION_FAILED)
- `field_path`: string - JSON path to problematic field
- `error_message`: string - Human-readable error description

### MQTTError
- `error_code`: int - MQTT-specific error code
- `error_message`: string - Error description
- `retry_possible`: bool - Whether operation can be retried
- `timestamp`: time_point - When error occurred

### TelemetryError
- `spot_id`: int - Which measurement spot had the error
- `error_type`: enum (SENSOR_ERROR, VALIDATION_FAILED, TRANSMISSION_FAILED)
- `temperature_value`: optional<double> - The problematic reading (if any)
- `error_message`: string - Detailed error information