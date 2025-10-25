# Data Model: Device Provisioning

**Feature**: Device Provisioning for ThingsBoard MQTT Client  
**Date**: 2025-10-23  
**Phase**: Design

## Core Entities

### ProvisioningRequest
Represents a device provisioning request to be sent to ThingsBoard.

**Fields**:
- `device_name: string` - Generated device name following pattern `thermal-camera-XXXXXX`
- `device_type: string` - Fixed value "thermal-camera" for device categorization
- `provision_device_key: string` - Provisioning authentication key from provision.json
- `provision_device_secret: string` - Provisioning authentication secret from provision.json
- `created_at: timestamp` - Request creation time for logging and correlation

**Validation Rules**:
- `device_name` must match pattern `^thermal-camera-[A-Z0-9]{6}$`
- `device_type` must equal "thermal-camera"
- `provision_device_key` must be non-empty string
- `provision_device_secret` must be non-empty string
- `created_at` must be valid timestamp

**Relationships**:
- Creates one ProvisioningResponse upon successful processing
- References ProvisioningCredentials for input data

### ProvisioningResponse
Represents the response received from ThingsBoard after provisioning request.

**Fields**:
- `status: string` - Response status ("SUCCESS" or "FAILURE")
- `device_id: string` - Unique ThingsBoard device identifier (UUID format)
- `device_name: string` - Confirmed device name (should match request)
- `access_token: string` - MQTT authentication token for device connections
- `credentials_type: string` - Authentication method (typically "ACCESS_TOKEN")
- `error_message: string` - Error description if status is FAILURE (optional)
- `error_code: string` - Structured error code for programmatic handling (optional)
- `received_at: timestamp` - Response reception time

**Validation Rules**:
- `status` must be either "SUCCESS" or "FAILURE"
- For SUCCESS responses: `device_id`, `device_name`, `access_token` must be non-empty
- For FAILURE responses: `error_message` should be provided
- `device_name` must match original request device name
- `access_token` must be valid base64-encoded string for SUCCESS responses

**Relationships**:
- Corresponds to one ProvisioningRequest
- Updates DeviceCredentials upon successful validation

### DeviceCredentials
Represents the final device credentials to be stored in thermal_config.json.

**Fields**:
- `device_id: string` - ThingsBoard device identifier from provisioning response
- `device_name: string` - Device name for identification
- `access_token: string` - MQTT authentication token
- `server_url: string` - ThingsBoard server URL (inherited from provisioning config)
- `server_port: int` - ThingsBoard MQTT port (inherited from provisioning config)
- `credentials_type: string` - Authentication method
- `provisioned_at: timestamp` - Successful provisioning completion time

**Validation Rules**:
- `device_id` must be valid UUID format
- `device_name` must match thermal-camera pattern
- `access_token` must be non-empty string
- `server_url` must be valid URL format
- `server_port` must be valid port number (1-65535)
- `provisioned_at` must be valid timestamp

**Relationships**:
- Created from successful ProvisioningResponse
- Replaces existing configuration in thermal_config.json

### ProvisioningCredentials
Represents the input credentials loaded from provision.json file.

**Fields**:
- `server_url: string` - ThingsBoard server URL for provisioning
- `server_port: int` - ThingsBoard MQTT port for provisioning
- `provision_device_key: string` - Provisioning authentication key
- `provision_device_secret: string` - Provisioning authentication secret
- `device_name_prefix: string` - Optional prefix override (default: "thermal-camera")
- `timeout_seconds: int` - Provisioning timeout (default: 30)

**Validation Rules**:
- `server_url` must be valid URL format
- `server_port` must be valid port number (1-65535)
- `provision_device_key` must be non-empty string
- `provision_device_secret` must be non-empty string
- `device_name_prefix` must match pattern `^[a-z-]+$` if provided
- `timeout_seconds` must be positive integer

**JSON Schema** (provision.json format):
```json
{
  "server_url": "eu.thingsboard.cloud",
  "server_port": 1883,
  "provision_device_key": "your-provision-key",
  "provision_device_secret": "your-provision-secret",
  "device_name_prefix": "thermal-camera",
  "timeout_seconds": 30
}
```

### ProvisioningStatus
Tracks the current state and progress of the provisioning process.

**Fields**:
- `current_state: enum` - Current provisioning state
- `started_at: timestamp` - Provisioning process start time
- `completed_at: timestamp` - Provisioning completion time (optional)
- `error_count: int` - Number of retry attempts made
- `last_error: string` - Last error message encountered (optional)
- `device_name: string` - Device name being provisioned
- `correlation_id: string` - Unique identifier for tracking logs

**State Transitions**:
```
IDLE -> DETECTING_FILES -> LOADING_CONFIG -> CONNECTING -> 
SENDING_REQUEST -> WAITING_RESPONSE -> VALIDATING_RESPONSE -> 
UPDATING_CONFIG -> COMPLETED

Error states: FAILED_CONFIG, FAILED_CONNECTION, FAILED_TIMEOUT, 
FAILED_VALIDATION, FAILED_UPDATE
```

**Validation Rules**:
- `current_state` must be valid enum value
- `started_at` must be valid timestamp
- `error_count` must be non-negative integer
- `correlation_id` must be unique identifier

## Data Flow Relationships

```
provision.txt (trigger) -> ProvisioningCredentials (from provision.json) ->
ProvisioningRequest (generated) -> [MQTT Communication] ->
ProvisioningResponse (received) -> DeviceCredentials (extracted) ->
thermal_config.json (updated)

ProvisioningStatus tracks the entire flow from start to completion
```

## Error Handling Data

### ProvisioningError
Represents structured error information for comprehensive error handling.

**Fields**:
- `error_type: enum` - Category of error (NETWORK, AUTH, VALIDATION, SERVER)
- `error_code: string` - Specific error identifier
- `error_message: string` - Human-readable error description
- `is_retryable: bool` - Whether the error should trigger retry logic
- `retry_after_seconds: int` - Suggested retry delay (optional)
- `occurred_at: timestamp` - Error occurrence time

**Error Types**:
- `NETWORK`: Connection, timeout, DNS resolution failures
- `AUTH`: Invalid credentials, expired tokens, permission issues
- `VALIDATION`: Malformed JSON, missing fields, invalid values
- `SERVER`: ThingsBoard internal errors, maintenance mode, quota exceeded

## Configuration File Structures

### Updated thermal_config.json
```json
{
  "device": {
    "device_id": "550e8400-e29b-41d4-a716-446655440000",
    "device_name": "thermal-camera-ABC123",
    "access_token": "your-device-access-token",
    "credentials_type": "ACCESS_TOKEN"
  },
  "server": {
    "url": "eu.thingsboard.cloud",
    "port": 1883,
    "use_ssl": true
  },
  "measurement_spots": [
    // existing measurement spot configuration
  ],
  "provisioning": {
    "provisioned_at": "2025-10-23T14:30:00Z",
    "provisioned_from": "provision.json"
  }
}
```

### Configuration Backup Naming
- Format: `thermal_config.json.backup.{timestamp}`
- Example: `thermal_config.json.backup.1698069000`
- Retention: Keep last 5 backups, clean up older files

## Memory Management

All data structures follow C++17 constitution requirements:
- Use `std::unique_ptr` for owned objects
- Use `std::shared_ptr` for shared ownership (JSON parsing results)
- Use references for non-owning access
- No raw pointer ownership
- RAII for resource cleanup (file handles, MQTT connections)

## Thread Safety

All provisioning data structures are designed for single-threaded access:
- No concurrent modification of provisioning state
- MQTT callbacks execute in single thread context
- File operations are atomic (write to temp, then rename)
- Timestamp generation uses thread-safe system calls