# Quickstart Guide: Thermal Camera MQTT Client

**Version**: 1.0.0  
**Last Updated**: 2025-10-23  
**Estimated Setup Time**: 15 minutes

## Prerequisites

### System Requirements
- **Operating System**: Linux (primary), macOS (secondary), Windows (optional)
- **Compiler**: GCC 7+ or Clang 5+ with C++17 support
- **Build Tools**: CMake 3.16+, Make or Ninja
- **Network**: Internet access for dependency download

### ThingsBoard Setup
1. **ThingsBoard Instance**: Access to ThingsBoard Community Edition or Cloud
2. **Device Access Token**: Device must be created in ThingsBoard with access token
3. **Network Access**: Firewall allows outbound connections to MQTT port (1883/8883)

## Quick Setup (5 minutes)

### 1. Clone and Build

```bash
# Clone the repository
git clone <repository-url>
cd tbclient

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)
```

### 2. Configure Connection

Create `config.json` in the project root:

```json
{
  "thingsboard": {
    "host": "mqtt.eu.thingsboard.cloud",
    "port": 1883,
    "access_token": "YOUR_DEVICE_ACCESS_TOKEN",
    "device_id": "thermal_camera_001"
  },
  "telemetry": {
    "interval_seconds": 15,
    "measurement_spots": [
      {"id": 1, "name": "engine", "x": 100, "y": 150, "min_temp": 60, "max_temp": 120, "noise_factor": 0.1, "enabled": true},
      {"id": 2, "name": "exhaust", "x": 200, "y": 300, "min_temp": 100, "max_temp": 200, "noise_factor": 0.2, "enabled": true}
    ]
  }
}
```

### 3. Run the Client

```bash
# From build directory
./thermal_mqtt_client ../config.json

# Or from project root
./build/thermal_mqtt_client config.json
```

### 4. Verify Operation

**Expected Console Output**:
```
[INFO] Loading configuration from config.json
[INFO] Connecting to ThingsBoard at your-thingsboard-host.com:1883
[INFO] Connected successfully with device ID: thermal_camera_001
[INFO] Starting telemetry transmission every 15 seconds
[INFO] Telemetry sent: {"spot": 1, "temperature": 85.7}
[INFO] Telemetry sent: {"spot": 2, "temperature": 156.2}
```

**ThingsBoard Dashboard**:
- Navigate to your device in ThingsBoard
- Check "Latest Telemetry" tab
- Verify temperature data appears for configured spots (sent as individual messages)

## Configuration Guide

### Minimal Configuration

```json
{
  "thingsboard": {
    "host": "thingsboard.cloud",
    "access_token": "YOUR_TOKEN"
  }
}
```

This uses defaults:
- Port: 1883 (non-SSL)
- Device ID: Generated from hostname
- Telemetry interval: 15 seconds
- One default measurement spot (max 5 spots supported)

### Complete Configuration

```json
{
  "thingsboard": {
    "host": "demo.thingsboard.io",
    "port": 1883,
    "access_token": "A1_TEST_TOKEN",
    "device_id": "thermal_camera_001",
    "use_ssl": false,
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
        "name": "engine_block",
        "x": 100,
        "y": 150,
        "min_temp": 60.0,
        "max_temp": 120.0,
        "noise_factor": 0.1,
        "enabled": true
      },
      {
        "id": 2,
        "name": "exhaust_manifold",
        "x": 200,
        "y": 300,
        "min_temp": 100.0,
        "max_temp": 200.0,
        "noise_factor": 0.2,
        "enabled": true
      }
    ]
  },
  "logging": {
    "level": "INFO",
    "console": true
  }
}
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `thingsboard.host` | string | Required | ThingsBoard server hostname |
| `thingsboard.port` | integer | 1883 | MQTT broker port |
| `thingsboard.access_token` | string | Required | Device access token |
| `thingsboard.device_id` | string | Auto-generated | Unique device identifier |
| `thingsboard.use_ssl` | boolean | false | Enable SSL/TLS encryption |
| `telemetry.interval_seconds` | integer | 15 | Seconds between transmissions |
| `telemetry.batch_transmission` | boolean | false | N/A - individual messages only |

## Usage Examples

### Basic Usage

```bash
# Run with default config.json
./thermal_mqtt_client

# Run with custom config file
./thermal_mqtt_client /path/to/my-config.json

# Run with verbose logging
./thermal_mqtt_client config.json --log-level DEBUG
```

### Command Line Options

```bash
Usage: thermal_mqtt_client [config_file] [options]

Options:
  -h, --help          Show this help message
  -v, --version       Show version information
  -l, --log-level     Set log level (DEBUG, INFO, WARN, ERROR)
  -t, --test-mode     Run in test mode (simulate readings only)
  --validate-config   Validate configuration and exit
```

### Test Mode

```bash
# Validate configuration without connecting
./thermal_mqtt_client config.json --validate-config

# Run with simulated readings (no MQTT connection)
./thermal_mqtt_client config.json --test-mode
```

## Troubleshooting

### Common Issues

#### 1. Connection Failed

**Error**: `Failed to connect to ThingsBoard`

**Solutions**:
- Check network connectivity: `ping your-thingsboard-host.com`
- Verify port is open: `telnet your-thingsboard-host.com 1883`
- Confirm access token is correct in ThingsBoard device settings
- Check firewall allows outbound connections on MQTT port

#### 2. Authentication Failed

**Error**: `Authentication failed (return code 4)`

**Solutions**:
- Verify access token is correct and active
- Check device exists in ThingsBoard
- Ensure device is not disabled
- Confirm access token has proper permissions
- Note: Client will stop retrying and require manual restart after auth failure

#### 3. Configuration Errors

**Error**: `Configuration validation failed`

**Solutions**:
- Validate JSON syntax with online JSON validator
- Check all required fields are present
- Verify numeric values are within valid ranges
- Ensure measurement spot IDs are unique
- Confirm maximum 5 measurement spots limit not exceeded

#### 4. Build Errors

**Error**: `Paho MQTT library not found`

**Solutions**:
- Ensure internet connection for dependency download
- Clear build directory and retry: `rm -rf build && mkdir build`
- Check CMake version: `cmake --version` (requires 3.16+)
- Verify compiler supports C++17: `gcc --version` or `clang --version`

### Debug Mode

Enable debug logging for detailed troubleshooting:

```json
{
  "logging": {
    "level": "DEBUG",
    "console": true
  }
}
```

**Debug Output Includes**:
- Detailed connection attempts
- MQTT message content
- Configuration parsing steps
- Temperature simulation details
- Retry attempt information

### Log Files

Logs are written to:
- **Console**: Immediate feedback
- **File**: `thermal_client.log` (if file logging enabled)

**Log Levels**:
- `ERROR`: Critical errors requiring attention
- `WARN`: Warnings that don't stop operation
- `INFO`: Normal operational information
- `DEBUG`: Detailed debugging information

## Getting Help

### Documentation
- **Full Specification**: `specs/001-thermal-mqtt-client/spec.md`
- **Implementation Plan**: `specs/001-thermal-mqtt-client/plan.md`
- **API Contracts**: `specs/001-thermal-mqtt-client/contracts/`

### ThingsBoard Resources
- **Community Edition**: https://thingsboard.io/docs/
- **Getting Started**: https://thingsboard.io/docs/getting-started-guides/
- **MQTT API**: https://thingsboard.io/docs/reference/mqtt-api/

### Support Checklist

When requesting support, please provide:
1. **Configuration file** (with sensitive tokens redacted)
2. **Complete error messages** from console output
3. **System information**: OS, compiler version, CMake version
4. **Build output** if compilation fails
5. **ThingsBoard version** and deployment type (cloud/on-premise)

## Next Steps

After successful setup:

1. **Monitor Telemetry**: Check ThingsBoard dashboard for incoming data
2. **Configure Alerts**: Set up ThingsBoard alarms for temperature thresholds
3. **Add More Spots**: Expand measurement_spots in configuration
4. **Enable SSL**: Switch to port 8883 with `use_ssl: true`
5. **Production Deployment**: Review security and reliability considerations

## Production Considerations

### Security
- Use SSL/TLS encryption (`use_ssl: true`, port 8883)
- Rotate access tokens regularly
- Restrict network access to necessary ports only
- Store configuration files with appropriate permissions

### Reliability
- Monitor client health and restart on failure
- Set up log rotation for long-running deployments
- Configure system service for automatic startup
- Implement monitoring and alerting for connection health

### Performance
- Adjust telemetry interval based on requirements
- Use batch transmission for multiple spots
- Monitor memory usage in long-running deployments
- Optimize measurement spot configuration for actual use case