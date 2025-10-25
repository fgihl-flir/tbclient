# Provisioning Setup Guide

This guide explains how to set up device provisioning for the Thermal Camera MQTT Client.

## Quick Start

1. **Copy the provisioning example file:**
   ```bash
   cp provision.example.json provision.json
   ```

2. **Edit provision.json with your ThingsBoard credentials:**
   - Replace `YOUR_PROVISION_DEVICE_KEY` with your actual provision device key
   - Replace `YOUR_PROVISION_DEVICE_SECRET` with your actual provision device secret  
   - Update `host` with your ThingsBoard server address
   - Adjust `port` if needed (default: 1883)

3. **Keep provision.txt in the application directory** (it's already there)

4. **Run the application:**
   ```bash
   ./build/thermal-mqtt-client
   ```

## What Happens During Provisioning

1. Application detects `provision.txt` file
2. Reads credentials from `provision.json`
3. Generates unique device name (e.g., `thermal-camera-7565`)
4. Connects to ThingsBoard provisioning endpoint
5. Registers new device and receives access token
6. Updates/creates `thermal_config.json` with new credentials
7. Removes `provision.txt` file
8. Application starts normal operation with new device credentials

## Configuration Files

| File | Purpose | When Created |
|------|---------|--------------|
| `provision.example.json` | Template for provisioning credentials | Included with project |
| `provision.json` | Your actual provisioning credentials | Copy from example |
| `provision.txt` | Trigger file for provisioning | Included with project |
| `thermal_config.json` | Device configuration after provisioning | Created automatically |
| `thermal_config.example.json` | Example of final device config | Included with project |

## Troubleshooting

- **Provisioning not triggered?** Check that `provision.txt` exists
- **Authentication failed?** Verify credentials in `provision.json`
- **Network errors?** Check ThingsBoard server address and port
- **File permissions?** Ensure application can read/write config files

## Security Notes

- Never commit `provision.json` with real credentials to version control
- The `provision.txt` file is automatically removed after successful provisioning
- Device access tokens are stored in `thermal_config.json` after provisioning