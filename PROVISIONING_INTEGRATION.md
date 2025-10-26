# Provisioning Integration in thermal-mqtt-client

## Overview

The **thermal-mqtt-client** now includes automatic device provisioning capability. When the application starts, it automatically detects if provisioning is needed and executes the complete workflow before normal operations begin.

## How It Works

### Startup Sequence

1. **Application Launch**: `thermal-mqtt-client` starts
2. **Provisioning Detection**: Checks for `provision.txt` file
3. **Mode Selection**:
   - If `provision.txt` exists → **Provisioning Mode**
   - If `provision.txt` doesn't exist → **Normal Operation Mode**

### Provisioning Mode (provision.txt exists)

When `provision.txt` is detected:

```
┌─────────────────────────────────────────────┐
│  1. Load provision.json credentials         │
│     - ThingsBoard server URL                │
│     - Provision device key & secret         │
│     - Timeout and retry settings            │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  2. Connect to ThingsBoard provisioning API │
│     - Generate unique device name           │
│     - Register device with server           │
│     - Receive access token                  │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  3. Create/Update thermal_config.json       │
│     - Save device credentials               │
│     - Configure ThingsBoard connection      │
│     - Set telemetry parameters              │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  4. Mark provisioning complete              │
│     - Rename provision.txt → .processed     │
│     - Display success message               │
│     - Exit (ready for restart)              │
└─────────────────────────────────────────────┘
```

### Normal Operation Mode (no provision.txt)

When `provision.txt` doesn't exist:

```
┌─────────────────────────────────────────────┐
│  1. Load thermal_config.json                │
│     - Device credentials (access token)     │
│     - ThingsBoard connection settings       │
│     - Telemetry configuration               │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  2. Connect to ThingsBoard                  │
│     - Authenticate with access token        │
│     - Subscribe to RPC topics               │
│     - Initialize spot management            │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  3. Run main application loop               │
│     - Send periodic telemetry               │
│     - Listen for RPC commands               │
│     - Manage thermal spots                  │
└─────────────────────────────────────────────┘
```

## Required Files

### For Provisioning

| File | Purpose | Required |
|------|---------|----------|
| `provision.txt` | Trigger file to enable provisioning | Yes |
| `provision.json` | Provisioning credentials from ThingsBoard | Yes |

### For Normal Operation

| File | Purpose | Required |
|------|---------|----------|
| `thermal_config.json` | Device configuration and credentials | Yes |
| `thermal_spots.json` | Persisted thermal spot data | No (auto-created) |

## Configuration Files

### provision.json Structure

```json
{
  "provisioning": {
    "device_key": "p6fqcbqj1o9en913nhn3",
    "device_secret": "yxa0i3xkt669swxo64i2",
    "host": "eu.thingsboard.cloud",
    "port": 1883,
    "timeout_seconds": 30,
    "max_retries": 3
  },
  "device": {
    "type": "thermal-camera",
    "profile": "loco-cam profile"
  }
}
```

### thermal_config.json (Auto-generated)

After successful provisioning, the application creates:

```json
{
  "thingsboard": {
    "host": "eu.thingsboard.cloud",
    "port": 1883,
    "access_token": "<generated-token>",
    "device_id": "thermal-camera-XXXXXX",
    "use_ssl": true,
    "keep_alive_seconds": 60,
    "qos_level": 1
  },
  "telemetry": {
    "interval_seconds": 15,
    "batch_transmission": false,
    "retry_attempts": 3,
    "retry_delay_ms": 1000,
    "measurement_spots": [
      {
        "id": 1,
        "name": "Center Spot",
        "x": 160,
        "y": 120,
        "min_temp": 20.0,
        "max_temp": 80.0,
        "noise_factor": 0.1,
        "enabled": true
      }
    ]
  },
  "logging": {
    "level": "info",
    "output": "console",
    "log_file": "thermal-mqtt.log"
  }
}
```

## Usage Examples

### First-Time Device Setup (Provisioning)

```bash
# 1. Ensure provision.json and provision.txt exist
cd /path/to/tbclient
ls provision.json provision.txt

# 2. Run the application
./build/thermal-mqtt-client

# Expected output:
# [INFO] Starting thermal camera MQTT client...
# [INFO] === Provisioning Mode Detected ===
# [INFO] Found provision.txt - Starting device provisioning workflow
# [INFO] ThingsBoard Server: eu.thingsboard.cloud:1883
# [INFO] ✓ Provisioning completed successfully!
# [INFO]   Device Name: thermal-camera-7B3F2A
# [INFO]   Access Token: 4h7sK9p2...
# [INFO]   Duration: 1234 ms
# [INFO]   Attempts: 1
# [INFO] 
# [INFO] thermal_config.json has been created/updated with device credentials
# [INFO] provision.txt has been marked as processed
# [INFO] You can now restart the application to connect with the new device credentials

# 3. Restart for normal operation
./build/thermal-mqtt-client

# Expected output:
# [INFO] Starting thermal camera MQTT client...
# [INFO] Configuration loaded successfully
# [INFO] ThingsBoard host: eu.thingsboard.cloud
# [INFO] Successfully connected to ThingsBoard
# [INFO] === Thermal Camera Ready for RPC Commands ===
```

### Normal Operation (Already Provisioned)

```bash
# provision.txt is now provision.txt.processed
./build/thermal-mqtt-client

# Application connects directly to ThingsBoard
# No provisioning step occurs
```

### Re-Provisioning a Device

```bash
# To provision a new device (get new credentials):
# 1. Rename the processed file back
mv provision.txt.processed provision.txt

# 2. Run the application
./build/thermal-mqtt-client

# The provisioning workflow will execute again
```

## Logging and Debugging

### Provisioning Success

```
[INFO] === Provisioning Mode Detected ===
[INFO] Found provision.txt - Starting device provisioning workflow
[INFO] ThingsBoard Server: eu.thingsboard.cloud:1883
[INFO] ✓ Provisioning completed successfully!
[INFO]   Device Name: thermal-camera-7B3F2A
[INFO]   Access Token: 4h7sK9p2...
[INFO]   Duration: 1234 ms
[INFO]   Attempts: 1
```

### Provisioning Failure

```
[ERROR] ✗ Provisioning failed!
[ERROR]   Error: Connection to ThingsBoard timed out
[ERROR]   Attempts: 3
[ERROR]   Duration: 90000 ms
[ERROR] 
[ERROR] Please check:
[ERROR]   1. provision.json contains valid credentials
[ERROR]   2. ThingsBoard server is accessible
[ERROR]   3. Network connectivity is working
```

### Normal Operation

```
[INFO] Configuration loaded successfully
[INFO] ThingsBoard host: eu.thingsboard.cloud
[INFO] Successfully connected to ThingsBoard
[INFO] === Thermal Camera Ready for RPC Commands ===
[INFO] Listening for RPC commands on: v1/devices/me/rpc/request/+
```

## Error Handling

### Common Errors and Solutions

| Error | Cause | Solution |
|-------|-------|----------|
| "Failed to load valid provisioning credentials" | Invalid or missing provision.json | Verify provision.json exists and has valid JSON |
| "Connection to ThingsBoard timed out" | Network or server issues | Check network connectivity and server URL |
| "Provisioning failed: authentication error" | Invalid device key/secret | Verify credentials in provision.json |
| "Failed to create thermal_config.json" | File permissions | Ensure write permissions in directory |

## Code Integration Points

### Main Application (src/main_paho.cpp)

The provisioning check is integrated at the very start of `main()`:

```cpp
int main() {
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    thermal::Logger::instance();
    
    try {
        // STEP 1: Check for provisioning
        if (std::filesystem::exists("./provision.txt")) {
            // Load provision.json credentials
            auto provision_creds = config::ProvisioningCredentials::loadFromFile("./provision.json");
            
            // Create workflow with correct broker details
            provisioning::ProvisioningWorkflow workflow(".", 
                provision_creds->getServerUrl(), 
                provision_creds->getServerPort());
            
            // Execute provisioning
            auto result = workflow.executeProvisioning();
            
            if (result.success) {
                LOG_INFO("Provisioning completed successfully!");
                return 0;  // Exit - restart needed
            } else {
                LOG_ERROR("Provisioning failed!");
                return 1;
            }
        }
        
        // STEP 2: Normal operation
        thermal::Configuration config;
        config.load_from_file("thermal_config.json");
        
        // ... continue with normal operations
    }
}
```

### Key Components

| Component | Location | Purpose |
|-----------|----------|---------|
| `ProvisioningWorkflow` | `src/provisioning/workflow.cpp` | Orchestrates complete provisioning process |
| `ProvisioningCredentials` | `src/config/provisioning.cpp` | Parses provision.json credentials |
| `ProvisioningClient` | `src/thingsboard/provisioning.cpp` | MQTT-based ThingsBoard provisioning |
| `ThermalConfigManager` | `src/config/provisioning.cpp` | Creates/updates thermal_config.json |

## Security Considerations

1. **Credential Storage**: 
   - `provision.json` contains sensitive keys - protect with file permissions
   - `thermal_config.json` contains access token - protect with file permissions

2. **Provisioning Files**:
   - Add to `.gitignore`: `provision.json`, `thermal_config.json`
   - Never commit real credentials to version control

3. **Network Security**:
   - Use SSL/TLS when `use_ssl: true` in provision.json
   - Provisioning over secure network recommended

## Testing

### Manual Test Sequence

```bash
# 1. Clean slate
rm -f thermal_config.json provision.txt.processed

# 2. Create provisioning trigger
touch provision.txt

# 3. Ensure valid provision.json exists
cat provision.json  # verify content

# 4. Run provisioning
./build/thermal-mqtt-client

# 5. Verify results
ls provision.txt.processed  # should exist
cat thermal_config.json     # should contain device credentials

# 6. Test normal operation
./build/thermal-mqtt-client

# Should connect and operate normally
```

### Integration with Tests

The provisioning workflow has comprehensive unit tests in:
- `tests/config/test_provisioning.cpp` - Credential parsing tests
- `tests/thingsboard/test_provisioning.cpp` - Provisioning client tests
- `tests/provisioning/test_workflow.cpp` - End-to-end workflow tests

## Benefits

✅ **Zero-Touch Deployment**: Drop application + provision files, device auto-registers  
✅ **Simplified Setup**: No manual credential copying or configuration editing  
✅ **Error Handling**: Clear error messages guide troubleshooting  
✅ **Idempotent**: Re-running provisioning is safe (creates new device)  
✅ **Production Ready**: Automatic backup of configurations, safe file operations  

## Troubleshooting Guide

### Provisioning Never Triggers

**Check**:
- Does `provision.txt` exist in the application directory?
- Does `provision.json` exist and contain valid JSON?

**Fix**:
```bash
touch provision.txt
cp provision.example.json provision.json
# Edit provision.json with real credentials
```

### Provisioning Fails Immediately

**Check**:
- Is `provision.json` valid JSON?
- Do credentials match ThingsBoard provisioning profile?

**Fix**:
```bash
# Validate JSON
cat provision.json | python3 -m json.tool

# Verify credentials with ThingsBoard admin
```

### Provisioning Hangs/Times Out

**Check**:
- Network connectivity to ThingsBoard server
- Firewall rules allow MQTT port (1883/8883)

**Fix**:
```bash
# Test connectivity
ping eu.thingsboard.cloud
nc -zv eu.thingsboard.cloud 1883
```

### thermal_config.json Not Created

**Check**:
- Write permissions in application directory
- Disk space available

**Fix**:
```bash
# Check permissions
ls -la
chmod u+w .

# Check disk space
df -h .
```

## Summary

The provisioning feature is now **fully integrated** into `thermal-mqtt-client`. When `provision.txt` exists, the application automatically:

1. Reads provisioning credentials from `provision.json`
2. Connects to ThingsBoard provisioning API
3. Registers new device and receives access token
4. Creates/updates `thermal_config.json` with credentials
5. Marks provisioning as complete
6. Exits (ready for restart with new credentials)

This enables **zero-touch deployment** for IoT devices in production environments.
