# JSON Configuration Contracts

**Feature**: Device Provisioning for ThingsBoard MQTT Client  
**Date**: 2025-10-23  
**Type**: Configuration File Schemas

## Input Configuration Contract

### provision.json Schema

**File**: `provision.json`  
**Purpose**: Provisioning credentials and configuration  
**Encoding**: UTF-8  
**Format**: JSON

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Device Provisioning Configuration",
  "type": "object",
  "required": ["server_url", "server_port", "provision_device_key", "provision_device_secret"],
  "properties": {
    "server_url": {
      "type": "string",
      "format": "hostname",
      "description": "ThingsBoard server hostname or IP address",
      "examples": ["eu.thingsboard.cloud", "localhost", "192.168.1.100"]
    },
    "server_port": {
      "type": "integer",
      "minimum": 1,
      "maximum": 65535,
      "default": 1883,
      "description": "ThingsBoard MQTT port number"
    },
    "provision_device_key": {
      "type": "string",
      "minLength": 1,
      "maxLength": 256,
      "description": "Provisioning authentication key from ThingsBoard"
    },
    "provision_device_secret": {
      "type": "string",
      "minLength": 1,
      "maxLength": 256,
      "description": "Provisioning authentication secret from ThingsBoard"
    },
    "device_name_prefix": {
      "type": "string",
      "pattern": "^[a-z][a-z0-9-]*$",
      "default": "thermal-camera",
      "description": "Optional device name prefix override"
    },
    "timeout_seconds": {
      "type": "integer",
      "minimum": 5,
      "maximum": 300,
      "default": 30,
      "description": "Provisioning timeout in seconds"
    },
    "use_ssl": {
      "type": "boolean",
      "default": true,
      "description": "Enable TLS/SSL encryption for MQTT connection"
    }
  },
  "additionalProperties": false
}
```

**Example**:
```json
{
  "server_url": "eu.thingsboard.cloud",
  "server_port": 1883,
  "provision_device_key": "your-provision-key-here",
  "provision_device_secret": "your-provision-secret-here",
  "device_name_prefix": "thermal-camera",
  "timeout_seconds": 30,
  "use_ssl": true
}
```

## Output Configuration Contract

### Updated thermal_config.json Schema

**File**: `thermal_config.json`  
**Purpose**: Device configuration with provisioned credentials  
**Encoding**: UTF-8  
**Format**: JSON

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Thermal Camera Device Configuration",
  "type": "object",
  "required": ["device", "server", "measurement_spots"],
  "properties": {
    "device": {
      "type": "object",
      "required": ["device_id", "device_name", "access_token", "credentials_type"],
      "properties": {
        "device_id": {
          "type": "string",
          "format": "uuid",
          "description": "Unique ThingsBoard device identifier"
        },
        "device_name": {
          "type": "string",
          "pattern": "^thermal-camera-[A-Z0-9]{6}$",
          "description": "Device name from provisioning"
        },
        "access_token": {
          "type": "string",
          "minLength": 1,
          "description": "MQTT authentication token"
        },
        "credentials_type": {
          "type": "string",
          "enum": ["ACCESS_TOKEN"],
          "description": "Authentication method"
        }
      },
      "additionalProperties": false
    },
    "server": {
      "type": "object",
      "required": ["url", "port"],
      "properties": {
        "url": {
          "type": "string",
          "format": "hostname",
          "description": "ThingsBoard server hostname"
        },
        "port": {
          "type": "integer",
          "minimum": 1,
          "maximum": 65535,
          "description": "ThingsBoard MQTT port"
        },
        "use_ssl": {
          "type": "boolean",
          "default": true,
          "description": "TLS/SSL encryption enabled"
        }
      },
      "additionalProperties": false
    },
    "measurement_spots": {
      "type": "array",
      "description": "Existing thermal measurement spot configuration",
      "items": {
        "type": "object"
      }
    },
    "provisioning": {
      "type": "object",
      "description": "Provisioning metadata (added after successful provisioning)",
      "properties": {
        "provisioned_at": {
          "type": "string",
          "format": "date-time",
          "description": "ISO 8601 timestamp of successful provisioning"
        },
        "provisioned_from": {
          "type": "string",
          "description": "Source of provisioning (typically 'provision.json')"
        },
        "backup_created": {
          "type": "string",
          "description": "Path to configuration backup file"
        }
      },
      "additionalProperties": false
    }
  },
  "additionalProperties": false
}
```

**Example**:
```json
{
  "device": {
    "device_id": "550e8400-e29b-41d4-a716-446655440000",
    "device_name": "thermal-camera-A1B2C3",
    "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
    "credentials_type": "ACCESS_TOKEN"
  },
  "server": {
    "url": "eu.thingsboard.cloud",
    "port": 1883,
    "use_ssl": true
  },
  "measurement_spots": [
    {
      "id": "spot-001",
      "name": "CPU Temperature",
      "position": {"x": 100, "y": 150},
      "enabled": true
    }
  ],
  "provisioning": {
    "provisioned_at": "2025-10-23T14:30:00Z",
    "provisioned_from": "provision.json",
    "backup_created": "thermal_config.json.backup.1698069000"
  }
}
```

## Configuration Update Contract

### Atomic Update Process
1. **Validation**: Validate new configuration against schema
2. **Backup**: Create timestamped backup of existing config
3. **Write**: Write new configuration to temporary file
4. **Validate**: Re-validate written configuration
5. **Atomic Move**: Rename temporary file to replace original
6. **Cleanup**: Remove temporary files on success

### Backup File Naming
- **Pattern**: `{original_filename}.backup.{unix_timestamp}`
- **Example**: `thermal_config.json.backup.1698069000`
- **Retention**: Keep last 5 backups, delete older files
- **Permissions**: Same as original file (typically 644)

### Error Handling
```json
{
  "validation_errors": {
    "missing_required_field": "Configuration missing required field: {field_name}",
    "invalid_format": "Field {field_name} has invalid format: {details}",
    "schema_violation": "Configuration violates schema: {error_details}"
  },
  "file_operation_errors": {
    "backup_failed": "Failed to create configuration backup: {error}",
    "write_failed": "Failed to write configuration file: {error}",
    "permission_denied": "Insufficient permissions to update configuration",
    "disk_full": "Insufficient disk space for configuration update"
  }
}
```

## Configuration Migration Contract

### Version Compatibility
- **Backward Compatibility**: New fields optional, existing fields preserved
- **Forward Compatibility**: Unknown fields ignored during parsing
- **Schema Evolution**: Graceful handling of schema changes

### Migration Strategy
```json
{
  "migration_steps": [
    {
      "from_version": "1.0",
      "to_version": "2.0",
      "transformations": [
        "Add provisioning section if missing",
        "Migrate device credentials format",
        "Update server configuration structure"
      ]
    }
  ]
}
```

## Security Considerations

### File Permissions
- **provision.json**: 600 (read/write owner only)
- **thermal_config.json**: 644 (read-only for group/others)
- **backup files**: Same as original file

### Sensitive Data Handling
- **Access Tokens**: Never logged in plain text
- **Provisioning Secrets**: Cleared from memory after use
- **Backup Files**: Same security as original configuration
- **Temporary Files**: Secure permissions during write process

### Validation Security
- **Input Sanitization**: All JSON fields validated against schema
- **Path Traversal Prevention**: File paths validated and constrained
- **Buffer Overflow Prevention**: String length limits enforced
- **Injection Prevention**: No dynamic code execution from configuration