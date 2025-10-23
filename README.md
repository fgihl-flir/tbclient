# Thermal Camera MQTT Client

A C++17 proof of concept MQTT client for thermal camera devices that connects to ThingsBoard.

## Features

- Connect to ThingsBoard MQTT broker
- Send temperature telemetry data every 15 seconds
- Support up to 5 measurement spots
- JSON configuration management
- Robust error handling and connection resilience

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

Copy `config.example.json` to `config.json` and edit with your ThingsBoard connection details:

```json
{
  "thingsboard": {
    "host": "localhost",
    "port": 1883,
    "access_token": "YOUR_ACCESS_TOKEN",
    "device_id": "thermal_camera_01"
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