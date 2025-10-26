# Thermal Camera Control via ThingsBoard RPC

## Overview

This document outlines the implementation strategy for remotely controlling thermal cameras through ThingsBoard's Remote Procedure Call (RPC) system. The focus is on enabling remote management of spot measurement functionality including creation, movement, and deletion of measurement points.

## ThingsBoard RPC System

### Architecture

ThingsBoard provides two types of RPC:
1. **Server-side RPC**: Platform sends commands to devices (our use case)
2. **Client-side RPC**: Devices send commands to platform

### MQTT Topics

For thermal camera control via MQTT:
- **Subscription Topic**: `v1/devices/me/rpc/request/+`
  - Device subscribes to receive incoming RPC commands
  - `+` is wildcard for request ID
- **Response Topic**: `v1/devices/me/rpc/response/{$request_id}`
  - Device publishes response using the specific request ID

### Message Format

RPC commands use JSON structure:
```json
{
  "method": "methodName",
  "params": {
    "parameter1": "value1",
    "parameter2": "value2"
  },
  "timeout": 30000
}
```

Device responses:
```json
{
  "result": "success",
  "data": {
    "response_data": "value"
  }
}
```

## Thermal Camera Control Commands

### 1. Create Spot Measurement

Creates a new measurement spot at specified coordinates.

**Command:**
```json
{
  "method": "createSpotMeasurement",
  "params": {
    "spotId": "1",
    "x": 160,
    "y": 120
  },
  "timeout": 5000
}
```

**Parameters:**
- `spotId`: Unique identifier for the spot, number 1 to 5
- `x`, `y`: Pixel coordinates on thermal image

**Response:**
```json
{
  "result": "success",
  "data": {
    "spotId": "1",
    "currentTemp": 25.3,
    "status": "active"
  }
}
```

### 2. Move Spot Measurement

Moves an existing measurement spot to new coordinates.

**Command:**
```json
{
  "method": "moveSpotMeasurement",
  "params": {
    "spotId": "1",
    "x": 180,
    "y": 140
  },
  "timeout": 5000
}
```

**Parameters:**
- `spotId`: Identifier of spot to move, number 1 to 5
- `x`, `y`: New pixel coordinates

**Response:**
```json
{
  "result": "success",
  "data": {
    "spotId": "1",
    "oldPosition": {"x": 160, "y": 120},
    "newPosition": {"x": 180, "y": 140},
    "currentTemp": 26.1
  }
}
```

### 3. Delete Spot Measurement

Removes a measurement spot from the thermal camera.

**Command:**
```json
{
  "method": "deleteSpotMeasurement",
  "params": {
    "spotId": "1"
  },
  "timeout": 5000
}
```

**Parameters:**
- `spotId`: Identifier of spot to delete, number 1 to 5

**Response:**
```json
{
  "result": "success",
  "data": {
    "spotId": "1",
    "status": "deleted"
  }
}
```

### 4. List All Spots

Retrieves information about all active measurement spots.

**Command:**
```json
{
  "method": "listSpotMeasurements",
  "params": {},
  "timeout": 5000
}
```

**Response:**
```json
{
  "result": "success",
  "data": {
    "spots": [
      {
        "spotId": "1",
        "x": 180,
        "y": 140,
        "currentTemp": 26.1,
        "status": "active"
      }
    ],
    "totalSpots": 1
  }
}
```

## Error Handling

### Common Error Responses

**Invalid Spot ID:**
```json
{
  "result": "error",
  "error": {
    "code": "SPOT_NOT_FOUND",
    "message": "Spot with ID '6' does not exist"
  }
}
```

**Invalid Coordinates:**
```json
{
  "result": "error",
  "error": {
    "code": "INVALID_COORDINATES",
    "message": "Coordinates (x=500, y=400) exceed image bounds (320x240)"
  }
}
```

**Camera Busy:**
```json
{
  "result": "error",
  "error": {
    "code": "CAMERA_BUSY",
    "message": "Camera is currently calibrating, try again later"
  }
}
```

## Implementation Requirements

### C++ Implementation Structure

```cpp
class ThermalCameraRPC {
private:
    mqtt::async_client& mqttClient;
    SpotMeasurementManager& spotManager;
    ThermalImageProcessor& imageProcessor;
    
public:
    // RPC command handlers
    void handleCreateSpot(const json& params, const std::string& requestId);
    void handleMoveSpot(const json& params, const std::string& requestId);
    void handleDeleteSpot(const json& params, const std::string& requestId);
    void handleListSpots(const json& params, const std::string& requestId);
    
    // MQTT callbacks
    void onRPCMessage(const std::string& topic, const std::string& payload);
    void sendRPCResponse(const std::string& requestId, const json& response);
};
```

### Spot Measurement Manager

```cpp
class SpotMeasurementManager {
private:
    std::map<std::string, SpotMeasurement> activeSpots;
    int maxSpots = 5; // Hardware limitation
    
public:
    bool createSpot(const std::string& spotId, int x, int y);
    bool moveSpot(const std::string& spotId, int newX, int newY);
    bool deleteSpot(const std::string& spotId);
    std::vector<SpotMeasurement> getAllSpots();
    
    // Temperature measurement
    double getCurrentTemperature(const std::string& spotId);
    bool isValidCoordinate(int x, int y);
};
```

### Configuration Requirements

#### thermal_config.json Extensions

```json
{
  "device": {
    "name": "thermal-camera-6760",
    "access_token": "QVOATLW5e06UoupcM4JP"
  },
  "thermal": {
    "resolution": {
      "width": 320,
      "height": 240
    },
    "temperature_range": {
      "min": -40.0,
      "max": 150.0
    },
    "spot_limits": {
      "max_spots": 5,
      "min_distance": 5
    }
  },
  "rpc": {
    "enabled": true,
    "timeout_ms": 5000,
    "topic_request": "v1/devices/me/rpc/request/+",
    "topic_response": "v1/devices/me/rpc/response/"
  }
}
```

## Security Considerations

### Authentication
- Device must authenticate with valid access token
- RPC commands are restricted to authenticated devices only

### Command Validation
- Validate all coordinates against image bounds
- Limit maximum number of simultaneous spots
- Sanitize spot names and IDs
- Validate emissivity values (0.0-1.0 range)

### Rate Limiting
- Implement command rate limiting to prevent abuse
- Queue commands if camera is busy processing

## Testing Strategy

### Unit Tests
1. **Command Parsing**: Validate JSON command parsing
2. **Coordinate Validation**: Test boundary conditions
3. **Spot Management**: Test create/move/delete operations
4. **Error Handling**: Test all error scenarios

### Integration Tests
1. **MQTT Communication**: Test RPC message flow
2. **ThingsBoard Integration**: Test with real ThingsBoard instance
3. **Thermal Camera Interface**: Test with actual hardware

## Sending Commands via ThingsBoard Web UI

### Accessing RPC Interface

1. **Login to ThingsBoard**
   - Navigate to your ThingsBoard instance (e.g., `demo.thingsboard.io`)
   - Login with your credentials

2. **Navigate to Device**
   - Go to **Devices** → **All**
   - Find your thermal camera device (e.g., `thermal-camera-6760`)
   - Click on the device name to open device details

3. **Access RPC Tab**
   - In the device details page, click on the **RPC** tab
   - This opens the Remote Procedure Call interface

### Sending RPC Commands

#### Method 1: Server-side RPC (Recommended)

1. **Select RPC Type**
   - Choose **Server-side RPC** (this sends commands from platform to device)
   - Set **Timeout** to `5000` ms

2. **Create Spot Measurement Example**
   ```json
   {
     "method": "createSpotMeasurement",
     "params": {
       "spotId": "1",
       "x": 160,
       "y": 120
     }
   }
   ```
   - Enter this JSON in the **Request** field
   - Click **Send RPC request**

3. **Move Spot Measurement Example**
   ```json
   {
     "method": "moveSpotMeasurement",
     "params": {
       "spotId": "1",
       "x": 180,
       "y": 140
     }
   }
   ```

4. **Delete Spot Measurement Example**
   ```json
   {
     "method": "deleteSpotMeasurement",
     "params": {
       "spotId": "1"
     }
   }
   ```

5. **List All Spots Example**
   ```json
   {
     "method": "listSpotMeasurements",
     "params": {}
   }
   ```

#### Method 2: Using RPC Widgets (Dashboard)

1. **Create Dashboard**
   - Go to **Dashboards** → **Add new dashboard**
   - Name it "Thermal Camera Control"

2. **Add RPC Widget**
   - Click **Add widget** → **Control widgets** → **RPC button** or **RPC shell**
   - Configure the widget with device and RPC method

3. **Configure RPC Button Widget**
   - **Target device**: Select your thermal camera
   - **RPC method**: `createSpotMeasurement`
   - **Request body**:
     ```json
     {
       "spotId": "1",
       "x": 160,
       "y": 120
     }
     ```
   - **Button label**: "Create Spot 1"

### Response Handling

**Successful Response Example:**
```json
{
  "result": "success",
  "data": {
    "spotId": "1",
    "currentTemp": 25.3,
    "status": "active"
  }
}
```

**Error Response Example:**
```json
{
  "result": "error",
  "error": {
    "code": "SPOT_NOT_FOUND",
    "message": "Spot with ID '6' does not exist"
  }
}
```

### UI Navigation Tips

1. **Device Status**: Check device **Latest telemetry** to see current spot temperatures
2. **Connection Status**: Verify device is **Active** before sending commands
3. **RPC History**: View previous RPC calls in the **Events** tab
4. **Real-time Data**: Use **Attributes** tab to see current spot configurations

### Troubleshooting

**Device Not Responding:**
- Verify device is connected (green dot in device list)
- Check device logs for MQTT connection issues
- Ensure device is subscribed to `v1/devices/me/rpc/request/+`

**RPC Timeout:**
- Increase timeout value (default 5000ms)
- Check network connectivity between device and ThingsBoard
- Verify device is processing RPC messages correctly

**Invalid Commands:**
- Validate JSON syntax in request
- Check parameter names match exactly
- Ensure spotId is between "1" and "5"
- Verify coordinates are within image bounds (0-319 x, 0-239 y)

### Test Commands

Example test sequence:
```bash
# Create test spot
mosquitto_pub -h localhost -t "v1/devices/me/rpc/request/123" \
  -m '{"method":"createSpotMeasurement","params":{"spotId":"1","x":160,"y":120}}'

# Move test spot
mosquitto_pub -h localhost -t "v1/devices/me/rpc/request/124" \
  -m '{"method":"moveSpotMeasurement","params":{"spotId":"1","x":180,"y":140}}'

# Delete test spot
mosquitto_pub -h localhost -t "v1/devices/me/rpc/request/125" \
  -m '{"method":"deleteSpotMeasurement","params":{"spotId":"1"}}'
```

## Thermal Imaging Fundamentals

### Measurement Spot Concepts

**Spot Measurement**: Point-based temperature measurement at specific pixel coordinates on thermal image.

**Emissivity**: Material property affecting thermal radiation measurement (0.0-1.0):
- High emissivity (0.95): Organic materials, painted surfaces
- Medium emissivity (0.5-0.8): Oxidized metals
- Low emissivity (0.02-0.1): Polished metals

**Temperature Accuracy**: Typical thermal cameras achieve ±2% accuracy under controlled conditions.

### Coordinate System

Thermal image coordinate system:
- Origin (0,0): Top-left corner
- X-axis: Horizontal, increases rightward
- Y-axis: Vertical, increases downward
- Bounds: (0,0) to (width-1, height-1)

For 320x240 resolution:
- Valid X range: 0-319
- Valid Y range: 0-239

## Future Enhancements

### Advanced Features
1. **Area Measurement**: Define rectangular or circular measurement areas
2. **Line Profiles**: Temperature profiles along defined lines
3. **Thermal Alarms**: Real-time temperature threshold monitoring
4. **Automatic Tracking**: Track moving hot spots automatically
5. **Multi-Point Calibration**: Enhanced accuracy through calibration points

### Analytics Integration
1. **Historical Data**: Store spot temperature history
2. **Trend Analysis**: Identify temperature patterns over time
3. **Predictive Alerts**: Predict equipment failures based on thermal trends
4. **Thermal Maps**: Generate heat maps for area analysis

## Conclusion

This RPC-based control system provides comprehensive remote management of thermal camera spot measurements through ThingsBoard's robust infrastructure. The implementation leverages MQTT for reliable communication and JSON for structured command/response data, ensuring scalable and maintainable thermal monitoring capabilities.

The system supports the core requirements:
- ✅ **Create spot measurement**: Define new measurement points
- ✅ **Move spot measurement**: Relocate existing measurement points  
- ✅ **Delete spot measurement**: Remove measurement points
- ✅ **List measurements**: Query all active measurement points

This foundation enables advanced thermal monitoring applications with real-time control and comprehensive data collection capabilities.