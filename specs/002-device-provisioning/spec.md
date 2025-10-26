# Feature Specification: Device Provisioning

**Feature Branch**: `002-device-provisioning`  
**Created**: 2025-10-23  
**Status**: Draft  
**Input**: User description: "Using our current mqtt client, we want add provisioning of a new client. The provision step should be executed if the file 'provision.txt' is found. the applicaiton should the provision a new device, using credential found in provision.json. When provision is done, update thermal_config.json with new device name and connection credential"

## Clarifications

### Session 2025-10-23

- Q: What authentication method should be used for the ThingsBoard provisioning API? → A: MQTT-based device provisioning using special provisioning credentials
- Q: What device naming pattern should be used when generating new device names? → A: thermal-camera-XXXXXX
- Q: What retry strategy should be used for network-related provisioning failures? → A: 3 retry attempts with 1s, 2s, 4s delays
- Q: How should `provision.txt` be handled after successful provisioning (rename to what or delete)? → A: provision.txt.processed
- Q: What strategy should be used to safely update `thermal_config.json` without corruption risk? → A: Create versioned backups with timestamps

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Automatic Device Provisioning (Priority: P1)

A system administrator or deployment script needs to automatically provision new thermal camera devices in a ThingsBoard system without manual intervention. When a new device is deployed, it should automatically register itself with the server and obtain its unique credentials.

**Why this priority**: This is the core functionality that enables automated device deployment and scaling. Without this, every new device requires manual configuration, which doesn't scale for IoT deployments.

**Independent Test**: Can be fully tested by placing a `provision.txt` file with valid `provision.json` credentials, running the application, and verifying that a new device is created in ThingsBoard and `thermal_config.json` is updated with the new credentials.

**Acceptance Scenarios**:

1. **Given** a new device deployment with `provision.txt` and `provision.json` files present, **When** the thermal MQTT client starts, **Then** it provisions a new device in ThingsBoard and updates `thermal_config.json` with the new device credentials
2. **Given** valid provisioning credentials in `provision.json`, **When** provisioning is triggered, **Then** the device receives a unique device ID and access token from ThingsBoard
3. **Given** successful provisioning completion, **When** the application restarts without `provision.txt`, **Then** it uses the newly provisioned credentials from `thermal_config.json` for normal operation

---

### User Story 2 - Provisioning Error Handling (Priority: P2)

When device provisioning fails due to network issues, invalid credentials, or server problems, the system should provide clear error messages and fail gracefully without corrupting existing configuration.

**Why this priority**: Robust error handling prevents devices from being left in unusable states during deployment and provides operators with actionable feedback.

**Independent Test**: Can be tested by providing invalid provisioning credentials or simulating network failures during provisioning, then verifying appropriate error messages and that existing configuration remains intact.

**Acceptance Scenarios**:

1. **Given** invalid provisioning credentials in `provision.json`, **When** provisioning is attempted, **Then** the system logs a clear error message and does not modify `thermal_config.json`
2. **Given** network connectivity issues during provisioning, **When** provisioning fails, **Then** the system retries with exponential backoff and provides status feedback
3. **Given** a malformed `provision.json` file, **When** the application starts, **Then** it logs validation errors and continues with existing configuration if available

---

### User Story 3 - Provisioning Status Monitoring (Priority: P3)

Operations teams need visibility into the provisioning process to troubleshoot deployment issues and monitor the status of device registration in large-scale deployments.

**Why this priority**: While not critical for basic functionality, monitoring capabilities are essential for production deployments and troubleshooting.

**Independent Test**: Can be tested by monitoring log output and checking for specific provisioning status messages during the provisioning process.

**Acceptance Scenarios**:

1. **Given** provisioning is in progress, **When** each major step completes, **Then** the system logs status updates with timestamps and relevant details
2. **Given** provisioning completes successfully, **When** the process finishes, **Then** the system logs the new device ID and confirms credential update
3. **Given** multiple provisioning attempts, **When** monitoring logs, **Then** each attempt is clearly distinguished with unique identifiers or timestamps

---

### Edge Cases

- What happens when `provision.txt` exists but `provision.json` is missing or malformed?
- How does the system handle partial provisioning failures (device created but credential update fails)?
- What occurs if `thermal_config.json` is read-only or the application lacks write permissions?
- How does the system behave if provisioning credentials are valid but the device name already exists in ThingsBoard?
- What happens if the application is interrupted during the provisioning process?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST detect the presence of `provision.txt` file at application startup
- **FR-002**: System MUST read provisioning credentials from `provision.json` when provisioning is triggered
- **FR-003**: System MUST validate the format and completeness of provisioning credentials before attempting provisioning
- **FR-004**: System MUST communicate with ThingsBoard using MQTT-based device provisioning with special provisioning credentials to create a new device
- **FR-005**: System MUST generate a unique device identifier using the pattern `thermal-camera-XXXXXX` during provisioning
- **FR-006**: System MUST obtain device access token and connection details from ThingsBoard
- **FR-007**: System MUST create a timestamped backup of `thermal_config.json` before updating it with new device credentials upon successful provisioning
- **FR-008**: System MUST rename `provision.txt` to `provision.txt.processed` after successful provisioning to prevent re-provisioning
- **FR-009**: System MUST log all provisioning activities with appropriate detail levels (info, warning, error)
- **FR-010**: System MUST preserve existing configuration in `thermal_config.json` using versioned backups if provisioning fails
- **FR-011**: System MUST implement retry logic with exponential backoff (3 attempts: 1s, 2s, 4s delays) for network-related provisioning failures
- **FR-012**: System MUST validate that newly provisioned credentials work before finalizing the configuration update

### Key Entities

- **Provisioning Request**: Contains server URL, provisioning credentials, device name template, and authentication details needed to register a new device
- **Device Credentials**: Includes device ID, access token, server connection details, and any device-specific configuration received from ThingsBoard
- **Provisioning Status**: Tracks the current state of provisioning (pending, in-progress, completed, failed) with timestamps and error details

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Device provisioning completes successfully within 30 seconds under normal network conditions
- **SC-002**: 99% of devices with valid provisioning credentials successfully register and obtain working access tokens
- **SC-003**: Provisioning failures provide actionable error messages within 10 seconds of detection
- **SC-004**: Zero configuration corruption occurs during failed provisioning attempts
- **SC-005**: Provisioning process can handle at least 100 concurrent device registrations without server overload
- **SC-006**: All provisioning activities are logged with sufficient detail to troubleshoot 95% of issues without additional debugging tools
