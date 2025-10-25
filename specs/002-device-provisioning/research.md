# Research Report: Device Provisioning

**Feature**: Device Provisioning for ThingsBoard MQTT Client  
**Date**: 2025-10-23  
**Research Phase**: Complete

## Research Tasks Completed

### 1. ThingsBoard MQTT Provisioning Protocol Flow

**Decision**: Use ThingsBoard Device Provisioning via MQTT with JSON-based request/response messages

**Rationale**: ThingsBoard supports MQTT-based device provisioning that aligns with our constitution requirement to use only MQTT (no HTTP APIs). The protocol uses standard MQTT publish/subscribe patterns with JSON payloads, which integrates seamlessly with our existing Eclipse Paho MQTT implementation.

**Alternatives considered**:
- HTTP REST API provisioning: Rejected due to constitution requirement for MQTT-only communication
- Manual device creation: Rejected as it doesn't meet automation requirements
- CoAP provisioning: Rejected as it requires additional protocol implementation

**Implementation Details**:
- Uses MQTT topic `/provision/request` for sending provisioning requests
- Receives responses on `/provision/response` topic
- JSON request format includes device name, device type, and provisioning credentials
- JSON response includes device ID, access token, and connection details

### 2. MQTT Topics and Message Structure

**Decision**: Follow ThingsBoard standard provisioning message format with device type "thermal-camera"

**Rationale**: Using standard ThingsBoard message format ensures compatibility and reduces custom protocol implementation. The device type "thermal-camera" clearly identifies the device category in ThingsBoard dashboards.

**Request Message Structure**:
```json
{
  "deviceName": "thermal-camera-XXXXXX",
  "deviceType": "thermal-camera",
  "provisionDeviceKey": "<from provision.json>",
  "provisionDeviceSecret": "<from provision.json>"
}
```

**Response Message Structure**:
```json
{
  "status": "SUCCESS",
  "deviceId": "<generated-device-id>",
  "deviceName": "thermal-camera-XXXXXX",
  "accessToken": "<device-access-token>",
  "credentialsType": "ACCESS_TOKEN"
}
```

**Alternative message formats**:
- Custom JSON structure: Rejected for compatibility reasons
- Binary protocol: Rejected for simplicity and debugging ease
- XML format: Rejected as JSON is ThingsBoard standard

### 3. Device Credentials Response Parsing

**Decision**: Parse JSON response using existing nlohmann/json library and extract device ID, access token, and credentials type

**Rationale**: Leverages existing JSON parsing infrastructure in the thermal camera client. The nlohmann/json library provides robust error handling and type safety for parsing responses.

**Response Fields to Extract**:
- `status`: SUCCESS/FAILURE indicator
- `deviceId`: Unique ThingsBoard device identifier
- `deviceName`: Confirmed device name (should match request)
- `accessToken`: MQTT authentication token for future connections
- `credentialsType`: Authentication method (typically "ACCESS_TOKEN")
- `errorMsg`: Error description if status is FAILURE

**Error Handling Strategy**:
- Missing required fields: Log error and abort provisioning
- Invalid JSON format: Log parse error and retry if network-related
- SUCCESS status with missing credentials: Log error and abort
- FAILURE status: Log specific error message and handle based on error type

**Alternatives considered**:
- Custom JSON parser: Rejected due to complexity and error-proneness
- String parsing: Rejected due to fragility and maintenance burden
- Binary response format: Not supported by ThingsBoard

### 4. Error Codes and Failure Modes

**Decision**: Implement comprehensive error handling covering network, authentication, validation, and ThingsBoard-specific errors

**Rationale**: Device provisioning involves multiple failure points that need different handling strategies. Network errors require retry, authentication errors need user action, and validation errors indicate configuration problems.

**Error Categories and Handling**:

1. **Network Errors** (Retriable with exponential backoff):
   - MQTT connection failures
   - Message delivery timeouts
   - Broker connectivity issues
   - DNS resolution failures

2. **Authentication Errors** (Not retriable, user action required):
   - Invalid provisioning credentials
   - Expired provisioning tokens
   - Insufficient permissions
   - Provisioning disabled on server

3. **Validation Errors** (Not retriable, configuration fix required):
   - Malformed provision.json
   - Missing required fields
   - Invalid device name pattern
   - Duplicate device name conflicts

4. **ThingsBoard-Specific Errors** (Context-dependent retry logic):
   - Provisioning quota exceeded: Retry with delay
   - Device already exists: Check if same device, handle accordingly
   - Server maintenance mode: Retry with longer delay
   - Internal server errors: Retry with backoff

**Error Response Format**:
```json
{
  "status": "FAILURE",
  "errorMsg": "Device with name 'thermal-camera-123456' already exists",
  "errorCode": "DEVICE_ALREADY_EXISTS"
}
```

**Implementation Strategy**:
- Use exception handling for programming errors (malformed JSON, null pointers)
- Use error codes for recoverable failures (network issues, temporary server problems)
- Log all errors with appropriate severity levels
- Provide user-friendly error messages with actionable guidance

## Best Practices Research

### MQTT Provisioning Security
- Always use TLS/SSL for provisioning communications
- Provisioning credentials should be single-use when possible
- Validate server certificates to prevent man-in-the-middle attacks
- Store received access tokens securely in configuration files

### Configuration File Management
- Create atomic file updates using temporary files and rename operations
- Implement configuration validation before applying updates
- Maintain versioned backups with timestamps for rollback capability
- Use file locking to prevent concurrent modification issues

### Device Naming Strategies
- Use consistent naming patterns for device identification
- Include device type prefix for easy categorization
- Generate random suffixes to ensure uniqueness
- Validate device names against ThingsBoard naming requirements

## Implementation Patterns

### Retry Logic Implementation
```cpp
class RetryManager {
    std::vector<int> delays = {1000, 2000, 4000}; // milliseconds
    int current_attempt = 0;
    
    bool should_retry() const {
        return current_attempt < delays.size();
    }
    
    int get_delay() {
        return delays[current_attempt++];
    }
};
```

### Configuration Backup Strategy
```cpp
class ConfigBackup {
    std::string create_backup(const std::string& config_path) {
        auto timestamp = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        auto backup_path = config_path + ".backup." + std::to_string(time_t);
        // Atomic copy operation
        return backup_path;
    }
};
```

### MQTT Message Handling
```cpp
class ProvisioningClient {
    void on_message_received(const std::string& topic, const std::string& payload) {
        if (topic == "/provision/response") {
            handle_provisioning_response(payload);
        }
    }
    
    void handle_provisioning_response(const std::string& response) {
        try {
            auto json = nlohmann::json::parse(response);
            if (json["status"] == "SUCCESS") {
                extract_credentials(json);
            } else {
                handle_provisioning_error(json);
            }
        } catch (const nlohmann::json::parse_error& e) {
            log_error("Failed to parse provisioning response: " + std::string(e.what()));
        }
    }
};
```

## Research Validation

All NEEDS CLARIFICATION items from Technical Context have been resolved:

✅ **ThingsBoard MQTT provisioning message format**: Standard JSON format defined  
✅ **MQTT topics and message structure**: `/provision/request` and `/provision/response` topics  
✅ **Response parsing for device credentials**: nlohmann/json extraction strategy  
✅ **Error codes and failure modes**: Comprehensive error categorization and handling

The research provides sufficient detail to proceed with Phase 1 design and implementation planning.