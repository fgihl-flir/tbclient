# Thermal Camera MQTT Client

A C++17 proof of concept MQTT client for thermal camera devices that connects to ThingsBoard.

## Features

- Connect to ThingsBoard MQTT broker
- Send temperature telemetry data every 15 seconds
- Support up to 5 measurement spots
- JSON configuration management
- Robust error handling and connection resilience
- **Automatic device provisioning** - Register new devices with ThingsBoard automatically

## Device Provisioning

The client supports automatic device provisioning with ThingsBoard. When a `provision.txt` file is detected in the application directory, the provisioning workflow is automatically triggered.

**📖 For detailed setup instructions, see [PROVISIONING.md](PROVISIONING.md)**

### How Provisioning Works

1. **Detection**: Application checks for `provision.txt` file on startup
2. **Configuration**: Reads provisioning credentials from `provision.json`
3. **Registration**: Automatically registers a new device with ThingsBoard
4. **Credential Update**: Updates `thermal_config.json` with new device credentials
5. **Backup**: Creates timestamped backup of original configuration

### Provisioning Setup

1. **Create provision.json** with your ThingsBoard provisioning credentials:
```bash
# Copy the example file and edit with your credentials
cp provision.example.json provision.json
```

Edit `provision.json` with your actual ThingsBoard provisioning credentials:
```json
{
  "provisioning": {
    "device_key": "YOUR_PROVISION_DEVICE_KEY",
    "device_secret": "YOUR_PROVISION_DEVICE_SECRET",
    "host": "your-thingsboard-server.com",
    "port": 1883,
    "timeout_seconds": 30,
    "max_retries": 3
  },
  "device": {
    "type": "thermal-camera",
    "profile": "thermal_sensor_profile"
  }
}
```

2. **Enable provisioning** by keeping the `provision.txt` file in your application directory:
```bash
# The provision.txt file is included - provisioning will trigger automatically
# Remove this file to disable provisioning
```

3. **Run the application** - it will automatically:
   - Generate a unique device name (e.g., `thermal-camera-7565`)
   - Provision the device with ThingsBoard
   - Update your `thermal_config.json` with new credentials
   - Remove `provision.txt` when complete

### Provisioning Configuration Files

- **provision.json**: Contains ThingsBoard provisioning server credentials
- **provision.txt**: Trigger file (presence indicates provisioning needed)
- **thermal_config.json**: Updated with new device credentials after successful provisioning

### Device Naming Convention

Provisioned devices follow the naming pattern: `thermal-camera-XXXX` where XXXX is a random 4-digit number for uniqueness.

### Error Handling

The provisioning system includes comprehensive error handling:
- Network connectivity issues
- Authentication failures
- Configuration validation errors
- File operation errors
- ThingsBoard server responses

Failed provisioning attempts are logged with detailed error messages for troubleshooting.

## Build Requirements

- CMake 3.16 or higher
- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- Internet connection (for downloading dependencies)

## Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure project
cmake ..

# Build
cmake --build .

# Run tests
ctest

# Run application
./thermal-mqtt-client
```

## Dependencies

- Eclipse Paho MQTT C++ (automatically downloaded and statically linked)
- nlohmann/json for configuration parsing
- Google Test for unit and integration testing

## Configuration

### Standard Configuration (Non-Provisioned Devices)

Copy `config.example.json` to `config.json` and edit with your ThingsBoard connection details:

### Provisioned Device Configuration

For devices using automatic provisioning, the `thermal_config.json` file will be automatically created and updated during the provisioning process. See `thermal_config.example.json` for the expected format:

```json
{
  "thingsboard": {
    "host": "your-thingsboard-server.com",
    "port": 1883,
    "access_token": "DEVICE_ACCESS_TOKEN_FROM_PROVISIONING",
    "device_id": "thermal-camera-1234"
  },
  "telemetry": {
    "interval_seconds": 15,
    "measurement_spots": [
      {
        "id": 1,
        "name": "Spot 1",
        "x": 100,
        "y": 100,
        "min_temp": 20.0,
        "max_temp": 100.0
      }
    ]
  }
}
```

## Architecture

- `src/thermal/` - Thermal camera simulation and measurement spot management
- `src/mqtt/` - MQTT client wrapper and connection management  
- `src/thingsboard/` - ThingsBoard-specific protocol and message handling
- `src/config/` - Configuration file parsing and management
- `src/common/` - Logging and error handling utilities
- `src/provisioning/` - Device provisioning workflow orchestration
- `src/utils/` - File operations and utility functions

### Provisioning Components

- `ProvisioningClient` - Main provisioning orchestrator
- `ProvisioningCredentials` - Provisioning server connection credentials
- `DeviceCredentials` - Device-specific access credentials
- `ThermalConfigManager` - Configuration file management and updates
- `ProvisioningWorkflow` - End-to-end provisioning process coordination
- `FileUtils` - File operations with backup and validation