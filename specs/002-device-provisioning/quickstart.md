# Quick Start: Device Provisioning

**Feature**: Device Provisioning for ThingsBoard MQTT Client  
**Date**: 2025-10-23  
**Audience**: Developers and System Administrators

## Overview

The Device Provisioning feature automatically registers new thermal camera devices with ThingsBoard when deployment files are detected. This enables zero-touch deployment for IoT devices in production environments.

## Prerequisites

- Existing thermal camera MQTT client installed and working
- ThingsBoard server with device provisioning enabled
- Provisioning credentials from ThingsBoard administrator
- Write permissions to configuration directory

## Quick Setup (5 minutes)

### 1. Obtain Provisioning Credentials

Contact your ThingsBoard administrator to get:
- Provisioning device key
- Provisioning device secret
- Server URL and port

### 2. Create Provisioning Configuration

Create `provision.json` file:

```json
{
  "server_url": "eu.thingsboard.cloud",
  "server_port": 1883,
  "provision_device_key": "your-provision-key-here",
  "provision_device_secret": "your-provision-secret-here",
  "timeout_seconds": 30,
  "use_ssl": true
}
```

### 3. Create Provisioning Trigger

Create empty `provision.txt` file to trigger provisioning:

```bash
touch provision.txt
```

### 4. Run Application

Start the thermal camera client normally:

```bash
./thermal-continuous-client
```

The application will automatically:
1. Detect `provision.txt` file
2. Load credentials from `provision.json`
3. Connect to ThingsBoard and provision device
4. Update `thermal_config.json` with new credentials
5. Rename `provision.txt` to `provision.txt.processed`
6. Continue normal operation with new device

### 5. Verify Success

Check the logs for successful provisioning:
```
[INFO] Provisioning mode detected
[INFO] Generated device name: thermal-camera-A1B2C3
[INFO] Connecting to ThingsBoard for provisioning...
[INFO] Device provisioned successfully
[INFO] Configuration updated with new credentials
[INFO] Provisioning complete, switching to normal operation
```

## Configuration Files

### provision.json (Input)
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

### thermal_config.json (Updated Output)
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
    // existing measurement configuration preserved
  ],
  "provisioning": {
    "provisioned_at": "2025-10-23T14:30:00Z",
    "provisioned_from": "provision.json"
  }
}
```

## Development Quick Start

### Building with Provisioning Support

The provisioning feature extends the existing thermal camera client. Build normally:

```bash
mkdir build && cd build
cmake ..
make
```

### Running Tests

Execute provisioning tests:

```bash
# Unit tests
./build/tests/unit/test_provisioning

# Integration tests (requires test ThingsBoard server)
./build/tests/integration/test_provisioning_integration
```

### Code Structure

Key files for provisioning implementation:

```text
src/
├── thingsboard/
│   └── provisioning.cpp        # Core provisioning logic
├── config/
│   └── provisioning.cpp        # Configuration parsing
└── utils/
    └── file_utils.cpp          # File backup and management

include/
├── thingsboard/
│   └── provisioning.h          # Provisioning interface
├── config/  
│   └── provisioning.h          # Configuration structures
└── utils/
    └── file_utils.h            # File utilities

tests/
├── unit/
│   └── test_provisioning.cpp   # Unit tests
└── integration/
    └── test_provisioning_integration.cpp # End-to-end tests
```

## Error Handling

### Common Issues and Solutions

**Error**: "Provisioning credentials invalid"
- **Cause**: Wrong provisioning key or secret
- **Solution**: Verify credentials with ThingsBoard administrator

**Error**: "Device name already exists"
- **Cause**: Generated device name conflicts with existing device
- **Solution**: Application will retry with new random suffix

**Error**: "Connection timeout during provisioning"
- **Cause**: Network connectivity issues
- **Solution**: Application retries with exponential backoff (1s, 2s, 4s)

**Error**: "Permission denied writing configuration"
- **Cause**: Insufficient file system permissions
- **Solution**: Ensure write permissions to configuration directory

### Debug Mode

Enable debug logging for troubleshooting:

```bash
export LOG_LEVEL=DEBUG
./thermal-continuous-client
```

Debug logs show:
- Provisioning file detection
- MQTT connection details
- Request/response messages
- Configuration update process

## Production Deployment

### Automated Deployment Script

```bash
#!/bin/bash
# deploy-thermal-device.sh

# Copy provisioning credentials
cp /secure/provision.json .
cp /secure/provision.txt .

# Set secure permissions
chmod 600 provision.json
chmod 644 provision.txt

# Start device (provisioning happens automatically)
./thermal-continuous-client

# Verify provisioning success
if [ -f "provision.txt.processed" ]; then
    echo "Device provisioned successfully"
    rm provision.json  # Clean up credentials
else
    echo "Provisioning failed"
    exit 1
fi
```

### Security Best Practices

1. **Credential Management**:
   - Store provisioning credentials securely
   - Use single-use credentials when possible
   - Remove `provision.json` after successful provisioning

2. **File Permissions**:
   - Set `provision.json` to 600 (owner read/write only)
   - Ensure configuration directory is writable
   - Backup files inherit secure permissions

3. **Network Security**:
   - Always use TLS/SSL for production (set `use_ssl: true`)
   - Validate server certificates
   - Use secure networks for provisioning

### Monitoring and Logging

Key metrics to monitor:
- Provisioning success rate
- Time to complete provisioning
- Network retry attempts
- Configuration backup creation

Log integration examples:
```bash
# Structured logging for monitoring systems
./thermal-continuous-client 2>&1 | grep "PROVISIONING" | tee provisioning.log

# Extract provisioning metrics
grep "Provisioning complete" /var/log/thermal-client.log | wc -l
```

## Troubleshooting

### Validation Steps

1. **Check File Presence**:
   ```bash
   ls -la provision.txt provision.json thermal_config.json
   ```

2. **Validate JSON Syntax**:
   ```bash
   python -m json.tool provision.json
   python -m json.tool thermal_config.json
   ```

3. **Test Network Connectivity**:
   ```bash
   telnet eu.thingsboard.cloud 1883
   ```

4. **Check Application Logs**:
   ```bash
   tail -f /var/log/thermal-client.log | grep -i provision
   ```

### Recovery Procedures

**Restore from Backup**:
```bash
# Find latest backup
ls -la thermal_config.json.backup.*

# Restore configuration
cp thermal_config.json.backup.1698069000 thermal_config.json
```

**Manual Re-provisioning**:
```bash
# Reset for re-provisioning
mv provision.txt.processed provision.txt
./thermal-continuous-client
```

### Support Information

For additional support:
- Check application logs in `/var/log/thermal-client.log`
- Review ThingsBoard server logs for provisioning errors
- Verify provisioning credentials with administrator
- Ensure network connectivity to ThingsBoard server
- Test with debug logging enabled for detailed troubleshooting