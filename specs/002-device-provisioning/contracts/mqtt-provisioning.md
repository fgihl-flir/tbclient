# MQTT Provisioning Protocol Contracts

**Feature**: Device Provisioning for ThingsBoard MQTT Client  
**Date**: 2025-10-23  
**Protocol**: MQTT-based ThingsBoard Device Provisioning

## MQTT Topic Contracts

### Provisioning Request Topic
**Topic**: `/provision/request`  
**Direction**: Client → ThingsBoard  
**QoS**: 1 (At least once delivery)  
**Retain**: false

### Provisioning Response Topic  
**Topic**: `/provision/response`  
**Direction**: ThingsBoard → Client  
**QoS**: 1 (At least once delivery)  
**Retain**: false

## Message Contracts

### Provisioning Request Message

**Content-Type**: `application/json`  
**Encoding**: UTF-8

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "ThingsBoard Provisioning Request",
  "type": "object",
  "required": ["deviceName", "deviceType", "provisionDeviceKey", "provisionDeviceSecret"],
  "properties": {
    "deviceName": {
      "type": "string",
      "pattern": "^thermal-camera-[A-Z0-9]{6}$",
      "description": "Unique device name following thermal camera naming convention"
    },
    "deviceType": {
      "type": "string",
      "enum": ["thermal-camera"],
      "description": "Device type for categorization in ThingsBoard"
    },
    "provisionDeviceKey": {
      "type": "string",
      "minLength": 1,
      "description": "Provisioning authentication key from provision.json"
    },
    "provisionDeviceSecret": {
      "type": "string",
      "minLength": 1,
      "description": "Provisioning authentication secret from provision.json"
    }
  },
  "additionalProperties": false
}
```

**Example**:
```json
{
  "deviceName": "thermal-camera-A1B2C3",
  "deviceType": "thermal-camera",
  "provisionDeviceKey": "example-provision-key-12345",
  "provisionDeviceSecret": "example-provision-secret-67890"
}
```

### Provisioning Success Response Message

**Content-Type**: `application/json`  
**Encoding**: UTF-8

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "ThingsBoard Provisioning Success Response",
  "type": "object",
  "required": ["status", "deviceId", "deviceName", "accessToken", "credentialsType"],
  "properties": {
    "status": {
      "type": "string",
      "enum": ["SUCCESS"],
      "description": "Response status indicating successful provisioning"
    },
    "deviceId": {
      "type": "string",
      "format": "uuid",
      "description": "Unique ThingsBoard device identifier"
    },
    "deviceName": {
      "type": "string",
      "pattern": "^thermal-camera-[A-Z0-9]{6}$",
      "description": "Confirmed device name (must match request)"
    },
    "accessToken": {
      "type": "string",
      "minLength": 1,
      "description": "MQTT authentication token for device connections"
    },
    "credentialsType": {
      "type": "string",
      "enum": ["ACCESS_TOKEN"],
      "description": "Authentication method type"
    }
  },
  "additionalProperties": false
}
```

**Example**:
```json
{
  "status": "SUCCESS",
  "deviceId": "550e8400-e29b-41d4-a716-446655440000",
  "deviceName": "thermal-camera-A1B2C3",
  "accessToken": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "credentialsType": "ACCESS_TOKEN"
}
```

### Provisioning Error Response Message

**Content-Type**: `application/json`  
**Encoding**: UTF-8

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "ThingsBoard Provisioning Error Response",
  "type": "object",
  "required": ["status", "errorMsg"],
  "properties": {
    "status": {
      "type": "string",
      "enum": ["FAILURE"],
      "description": "Response status indicating provisioning failure"
    },
    "errorMsg": {
      "type": "string",
      "minLength": 1,
      "description": "Human-readable error description"
    },
    "errorCode": {
      "type": "string",
      "enum": [
        "INVALID_CREDENTIALS",
        "DEVICE_ALREADY_EXISTS", 
        "QUOTA_EXCEEDED",
        "PROVISIONING_DISABLED",
        "INVALID_DEVICE_NAME",
        "INTERNAL_ERROR"
      ],
      "description": "Structured error code for programmatic handling"
    }
  },
  "additionalProperties": false
}
```

**Example**:
```json
{
  "status": "FAILURE",
  "errorMsg": "Device with name 'thermal-camera-A1B2C3' already exists",
  "errorCode": "DEVICE_ALREADY_EXISTS"
}
```

## Connection Contract

### MQTT Connection Parameters
- **Protocol**: MQTT 3.1.1
- **Transport**: TCP with optional TLS/SSL
- **Client ID**: `provisioning-client-{random-suffix}`
- **Clean Session**: true
- **Keep Alive**: 60 seconds
- **Connect Timeout**: 30 seconds
- **Username**: provisioning device key
- **Password**: provisioning device secret

### TLS/SSL Requirements
- **TLS Version**: 1.2 or higher
- **Certificate Validation**: Required for production
- **Cipher Suites**: Strong encryption only
- **Hostname Verification**: Required

## Error Handling Contracts

### Network Error Responses
When MQTT communication fails, the client must handle these scenarios:

1. **Connection Timeout**: Retry with exponential backoff
2. **Authentication Failure**: Log error and abort (no retry)
3. **Topic Permission Denied**: Log error and abort (no retry)
4. **Message Delivery Failure**: Retry up to 3 times
5. **Broker Unavailable**: Retry with exponential backoff

### Response Timeout Contract
- **Initial Timeout**: 30 seconds
- **Retry Strategy**: 3 attempts with 1s, 2s, 4s delays
- **Maximum Total Time**: 45 seconds (30 + 1 + 2 + 4 + 8 seconds buffer)

### Error Classification
```json
{
  "NETWORK_ERRORS": [
    "CONNECTION_TIMEOUT",
    "CONNECTION_REFUSED", 
    "HOSTNAME_RESOLUTION_FAILED",
    "CERTIFICATE_VERIFICATION_FAILED"
  ],
  "AUTHENTICATION_ERRORS": [
    "INVALID_CREDENTIALS",
    "EXPIRED_CREDENTIALS",
    "INSUFFICIENT_PERMISSIONS"
  ],
  "VALIDATION_ERRORS": [
    "INVALID_DEVICE_NAME",
    "MALFORMED_REQUEST",
    "MISSING_REQUIRED_FIELDS"
  ],
  "SERVER_ERRORS": [
    "DEVICE_ALREADY_EXISTS",
    "QUOTA_EXCEEDED", 
    "PROVISIONING_DISABLED",
    "INTERNAL_ERROR"
  ]
}
```

## Quality of Service Contracts

### Message Delivery Guarantees
- **QoS 1**: At least once delivery for both request and response
- **Message Ordering**: Not guaranteed across different MQTT sessions
- **Duplicate Detection**: Client must handle duplicate responses
- **Message Persistence**: Broker may persist messages during disconnection

### Performance Contracts
- **Request Processing Time**: Maximum 10 seconds under normal load
- **Response Size**: Maximum 1KB for success, 2KB for error responses
- **Concurrent Requests**: Server supports up to 100 simultaneous provisioning requests
- **Rate Limiting**: Maximum 10 provisioning requests per minute per client

## Security Contracts

### Authentication Flow
1. Client connects with provisioning credentials (username/password)
2. Client publishes request to `/provision/request` topic
3. Client subscribes to `/provision/response` topic  
4. Server validates credentials and processes request
5. Server publishes response to `/provision/response` topic
6. Client processes response and disconnects

### Credential Security
- **Provisioning Credentials**: Single-use preferred, limited lifetime
- **Access Tokens**: Long-lived, device-specific authentication
- **Transport Security**: TLS encryption required for credential exchange
- **Credential Storage**: Secure local storage, file permissions 600

### Message Security
- **Payload Encryption**: TLS transport encryption
- **Message Integrity**: MQTT protocol-level integrity checks
- **Replay Protection**: Timestamp validation and correlation IDs
- **Information Disclosure**: No sensitive data in error messages