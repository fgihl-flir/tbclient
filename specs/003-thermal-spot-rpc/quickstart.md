# Quick Start Guide: Thermal Camera Spot Measurement RPC Control

**Date**: 26 October 2025  
**Audience**: Developers and operators testing thermal spot RPC functionality  
**Prerequisites**: ThingsBoard device provisioned, MQTT client tools installed

## Overview

This guide demonstrates how to test thermal camera spot measurement control via ThingsBoard RPC commands. You'll create, move, delete, and list temperature measurement spots using MQTT commands.

## Prerequisites

### 1. Device Setup
- Thermal camera device provisioned in ThingsBoard  
- Device access token obtained from ThingsBoard
- Device connected and sending telemetry data
- MQTT client tools installed (`mosquitto_pub`, `mosquitto_sub`)

### 2. Configuration Check
Verify your device configuration in `thermal_config.json`:
```json
{
  "device": {
    "name": "thermal-camera-6760",
    "access_token": "YOUR_ACCESS_TOKEN_HERE"
  },
  "thermal": {
    "resolution": {
      "width": 320,
      "height": 240
    }
  }
}
```

### 3. ThingsBoard Access
- ThingsBoard instance URL (e.g., `demo.thingsboard.io`)
- Admin/tenant credentials for monitoring
- Device dashboard access for telemetry viewing

## Quick Test Sequence (5 minutes)

### Step 1: Monitor RPC Responses
Open a terminal and subscribe to device responses:
```bash
mosquitto_sub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/response/+" \
  -v
```
*Keep this terminal open to see all responses*

### Step 2: Create First Measurement Spot
Open a second terminal and create spot at image center:
```bash
mosquitto_pub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/request/test001" \
  -m '{
    "method": "createSpotMeasurement",
    "params": {
      "spotId": "1", 
      "x": 160,
      "y": 120
    }
  }'
```

**Expected Response:**
```json
{
  "result": "success",
  "data": {
    "spotId": "1",
    "coordinates": {"x": 160, "y": 120},
    "currentTemp": 25.3,
    "status": "active"
  }
}
```

### Step 3: Create Second Spot
```bash
mosquitto_pub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/request/test002" \
  -m '{
    "method": "createSpotMeasurement", 
    "params": {
      "spotId": "2",
      "x": 200,
      "y": 100
    }
  }'
```

### Step 4: List All Active Spots
```bash
mosquitto_pub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/request/test003" \
  -m '{
    "method": "listSpotMeasurements",
    "params": {}
  }'
```

**Expected Response:**
```json
{
  "result": "success",
  "data": {
    "spots": [
      {
        "spotId": "1",
        "coordinates": {"x": 160, "y": 120},
        "currentTemp": 25.3,
        "status": "active"
      },
      {
        "spotId": "2", 
        "coordinates": {"x": 200, "y": 100},
        "currentTemp": 28.1,
        "status": "active"
      }
    ],
    "totalSpots": 2
  }
}
```

### Step 5: Move First Spot
```bash
mosquitto_pub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/request/test004" \
  -m '{
    "method": "moveSpotMeasurement",
    "params": {
      "spotId": "1",
      "x": 180,
      "y": 140
    }
  }'
```

**Expected Response:**
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

### Step 6: Delete Second Spot
```bash
mosquitto_pub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/request/test005" \
  -m '{
    "method": "deleteSpotMeasurement",
    "params": {
      "spotId": "2"
    }
  }'
```

**Expected Response:**
```json
{
  "result": "success", 
  "data": {
    "spotId": "2",
    "status": "deleted"
  }
}
```

### Step 7: Verify Final State
```bash
mosquitto_pub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/request/test006" \
  -m '{
    "method": "listSpotMeasurements",
    "params": {}
  }'
```

**Expected Response:**
```json
{
  "result": "success",
  "data": {
    "spots": [
      {
        "spotId": "1",
        "coordinates": {"x": 180, "y": 140}, 
        "currentTemp": 26.1,
        "status": "active"
      }
    ],
    "totalSpots": 1
  }
}
```

## Testing Error Scenarios

### Test Invalid Coordinates
```bash
mosquitto_pub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/request/error001" \
  -m '{
    "method": "createSpotMeasurement",
    "params": {
      "spotId": "3",
      "x": 400,
      "y": 300
    }
  }'
```

**Expected Error Response:**
```json
{
  "result": "error",
  "error": {
    "code": "INVALID_COORDINATES",
    "message": "Coordinates (x=400, y=300) exceed image bounds (320x240)"
  }
}
```

### Test Duplicate Spot Creation
```bash
mosquitto_pub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/request/error002" \
  -m '{
    "method": "createSpotMeasurement",
    "params": {
      "spotId": "1",
      "x": 100,
      "y": 100
    }
  }'
```

**Expected Error Response:**
```json
{
  "result": "error",
  "error": {
    "code": "SPOT_ALREADY_EXISTS", 
    "message": "Spot with ID '1' already exists"
  }
}
```

### Test Non-Existent Spot Operations
```bash
mosquitto_pub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/request/error003" \
  -m '{
    "method": "moveSpotMeasurement",
    "params": {
      "spotId": "9",
      "x": 100,
      "y": 100  
    }
  }'
```

**Expected Error Response:**
```json
{
  "result": "error",
  "error": {
    "code": "SPOT_NOT_FOUND",
    "message": "Spot with ID '9' does not exist"
  }
}
```

## ThingsBoard Web UI Testing

### Method 1: RPC Tab Testing

1. **Access Device RPC**
   - Login to ThingsBoard web interface
   - Navigate to **Devices** → **All** 
   - Click on your thermal camera device
   - Select **RPC** tab

2. **Send Commands via UI**
   - Select **Server-side RPC**
   - Set timeout to `5000` ms
   - Enter command JSON in request field
   - Click **Send RPC request**

3. **Example UI Command**
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

### Method 2: Dashboard Widget Testing

1. **Create Dashboard**
   - Go to **Dashboards** → **Add new dashboard**
   - Name: "Thermal Camera Control"

2. **Add RPC Button Widget**
   - **Add widget** → **Control widgets** → **RPC button**
   - **Target device**: Your thermal camera
   - **RPC method**: `createSpotMeasurement`
   - **Request body**: `{"spotId": "1", "x": 160, "y": 120}`
   - **Button label**: "Create Spot 1"

3. **Add Multiple Control Buttons**
   - Create buttons for each RPC operation
   - Use different spotIds for testing
   - Add list button with empty params: `{}`

## Monitoring and Debugging

### 1. Device Telemetry Monitoring
Check ThingsBoard device telemetry for spot temperature data:
- **Latest telemetry** tab: Current spot temperatures
- **Telemetry charts**: Temperature trends over time
- **Attributes** tab: Current spot configurations

### 2. MQTT Traffic Analysis
Monitor raw MQTT traffic:
```bash
# Subscribe to all device topics
mosquitto_sub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/#" \
  -v

# Monitor only RPC traffic
mosquitto_sub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/+" \
  -v
```

### 3. Device Logs Analysis
Check thermal camera application logs:
```bash
# View recent logs
tail -f /var/log/thermal-camera.log

# Search for RPC-related entries
grep "RPC" /var/log/thermal-camera.log

# Check error patterns  
grep "ERROR\|WARN" /var/log/thermal-camera.log
```

### 4. File Persistence Verification
Check spot persistence file:
```bash
# View current spots configuration
cat config/thermal_spots.json

# Monitor file changes
watch -n 1 cat config/thermal_spots.json

# Check file permissions
ls -la config/thermal_spots.json
```

## Performance Testing

### 1. Response Time Testing
```bash
# Time command execution
time mosquitto_pub -h demo.thingsboard.io -p 1883 \
  -u YOUR_ACCESS_TOKEN \
  -t "v1/devices/me/rpc/request/perf001" \
  -m '{"method":"createSpotMeasurement","params":{"spotId":"1","x":160,"y":120}}'
```

### 2. Concurrent Spot Testing
Create all 5 spots quickly to test maximum capacity:
```bash
for i in {1..5}; do
  mosquitto_pub -h demo.thingsboard.io -p 1883 \
    -u YOUR_ACCESS_TOKEN \
    -t "v1/devices/me/rpc/request/multi00$i" \
    -m "{\"method\":\"createSpotMeasurement\",\"params\":{\"spotId\":\"$i\",\"x\":$((100+i*30)),\"y\":$((80+i*20))}}"
  sleep 0.1
done
```

### 3. Rapid Command Testing
Test sequential command processing:
```bash
# Send commands without delay
mosquitto_pub -h demo.thingsboard.io -p 1883 -u YOUR_ACCESS_TOKEN -t "v1/devices/me/rpc/request/rapid001" -m '{"method":"createSpotMeasurement","params":{"spotId":"1","x":160,"y":120}}'
mosquitto_pub -h demo.thingsboard.io -p 1883 -u YOUR_ACCESS_TOKEN -t "v1/devices/me/rpc/request/rapid002" -m '{"method":"moveSpotMeasurement","params":{"spotId":"1","x":180,"y":140}}'
mosquitto_pub -h demo.thingsboard.io -p 1883 -u YOUR_ACCESS_TOKEN -t "v1/devices/me/rpc/request/rapid003" -m '{"method":"listSpotMeasurements","params":{}}'
```

## Troubleshooting Common Issues

### Device Not Responding
**Symptoms**: No response to RPC commands
**Solutions**:
1. Check device connection status in ThingsBoard
2. Verify MQTT broker connectivity: `mosquitto_pub -h demo.thingsboard.io -p 1883 -u YOUR_ACCESS_TOKEN -t "v1/devices/me/telemetry" -m '{"test":1}'`
3. Check device logs for MQTT subscription errors
4. Verify access token is correct

### Invalid JSON Responses
**Symptoms**: `INVALID_JSON` error responses
**Solutions**:
1. Validate JSON syntax: `echo '{"method":"test"}' | python -m json.tool`
2. Check for special characters in command
3. Ensure proper escaping in shell commands
4. Use single quotes around JSON in bash

### Timeout Errors
**Symptoms**: No response within timeout period
**Solutions**:
1. Increase timeout value to 10000ms
2. Check device processing load
3. Verify thermal camera hardware responsiveness
4. Monitor system resources (CPU, memory)

### Coordinate Validation Errors
**Symptoms**: `INVALID_COORDINATES` for valid-looking coordinates
**Solutions**:
1. Verify image resolution is 320x240
2. Check coordinate ranges: x(0-319), y(0-239)
3. Ensure coordinates are integers, not floats
4. Test with known good coordinates: (160, 120)

## Success Criteria Verification

Use this checklist to verify implementation meets requirements:

- [ ] **SC-001**: Create spot responds within 2 seconds
- [ ] **SC-002**: All valid commands process without errors  
- [ ] **SC-003**: Move spot completes within 1 second
- [ ] **SC-006**: Temperature readings available within 500ms
- [ ] **SC-007**: All 5 spots can be managed concurrently

## Next Steps

After successful testing:
1. **Integration Testing**: Test with real thermal camera hardware
2. **Load Testing**: Sustained operation with continuous commands
3. **Failure Testing**: Network interruption and recovery scenarios
4. **Production Deployment**: Configure for production ThingsBoard instance

This quick start guide provides comprehensive testing coverage for thermal spot RPC functionality, enabling rapid validation of implementation correctness and performance characteristics.