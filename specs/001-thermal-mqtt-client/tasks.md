# Tasks: Thermal Camera MQTT Client

**Input**: Design documents from `/specs/001-thermal-mqtt-client/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

**Tests**: Tests are included as specified in requirements for 80%+ test coverage.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **C++ project**: `src/`, `include/`, `tests/` at repository root
- `src/` contains implementation files (.cpp)
- `include/` contains public header files (.h/.hpp)
- `tests/` contains unit and integration tests
- CMakeLists.txt for build configuration

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: C++ project initialization and build system setup

- [x] T001 Create CMake project structure with src/, include/, tests/ directories
- [x] T002 Initialize CMakeLists.txt with C++17 standard and Eclipse Paho MQTT dependency
- [x] T003 [P] Configure compiler flags (-Wall -Wextra -Werror) and Google Test integration

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core C++ infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

Essential foundational tasks for C++ MQTT client:

- [x] T004 Implement configuration management class for JSON config files in src/config/
- [x] T005 [P] Create MQTT connection wrapper with async client in src/mqtt/
- [x] T006 [P] Setup logging infrastructure with configurable levels in src/common/
- [x] T007 Create base MQTT message classes and ThingsBoard protocol handlers in src/thingsboard/
- [x] T008 Implement connection resilience with exponential backoff retry logic in src/mqtt/
- [x] T009 Setup error handling framework with C++ exceptions in src/common/

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Basic MQTT Connection (Priority: P1) 🎯 MVP

**Goal**: Establish MQTT connection to ThingsBoard and send temperature data from one measurement spot

**Independent Test**: Connect to ThingsBoard, send a temperature reading, verify message appears in device telemetry

### Tests for User Story 1

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [x] T010 [P] [US1] Unit test for MQTTClient connection management in tests/unit/test_mqtt_client.cpp
- [x] T011 [P] [US1] Unit test for ThingsBoardDevice authentication in tests/unit/test_thingsboard_device.cpp
- [x] T012 [P] [US1] Integration test for broker connectivity in tests/integration/test_connection.cpp
- [x] T013 [P] [US1] Unit test for TemperatureReading validation in tests/unit/test_temperature_reading.cpp

### Implementation for User Story 1

- [x] T014 [P] [US1] Create MQTTClient class header in include/mqtt/client.h
- [x] T015 [P] [US1] Create ThingsBoardDevice class header in include/thingsboard/device.h
- [x] T016 [P] [US1] Create TemperatureReading entity in include/thermal/temperature_reading.h
- [x] T017 [US1] Implement MQTTClient class in src/mqtt/client.cpp (depends on T014)
- [x] T018 [US1] Implement ThingsBoardDevice in src/thingsboard/device.cpp (depends on T015, T017)
- [x] T019 [US1] Implement TemperatureReading with validation in src/thermal/temperature_reading.cpp
- [x] T020 [US1] Add structured logging for MQTT operations and connection events
- [x] T021 [US1] Implement main application entry point in src/main.cpp

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently

---

## Phase 4: User Story 2 - Multiple Measurement Spots (Priority: P2)

**Goal**: Support up to 5 measurement spots, each sending temperature data independently

**Independent Test**: Configure multiple spots, verify each can send data independently with correct spot identification

### Tests for User Story 2

- [ ] T022 [P] [US2] Unit test for MeasurementSpot configuration in tests/unit/test_measurement_spot.cpp
- [ ] T023 [P] [US2] Unit test for TelemetryMessage formatting in tests/unit/test_telemetry_message.cpp
- [ ] T024 [P] [US2] Integration test for multi-spot telemetry in tests/integration/test_multi_spot.cpp

### Implementation for User Story 2

- [x] T025 [P] [US2] Create MeasurementSpot entity in include/thermal/measurement_spot.h
- [ ] T026 [P] [US2] Create TelemetryMessage class in include/thingsboard/telemetry_message.h
- [ ] T027 [P] [US2] Create ThermalManager class header in include/thermal/thermal_manager.h
- [x] T028 [US2] Implement MeasurementSpot with validation in src/thermal/measurement_spot.cpp
- [ ] T029 [US2] Implement TelemetryMessage formatting in src/thingsboard/telemetry_message.cpp
- [ ] T030 [US2] Implement ThermalManager for spot coordination in src/thermal/thermal_manager.cpp
- [ ] T031 [US2] Integrate multi-spot support into main application

**Checkpoint**: At this point, User Stories 1 AND 2 should both work independently

---

## Phase 5: User Story 3 - JSON Configuration Management (Priority: P3)

**Goal**: Load connection settings and measurement spot configurations from JSON files

**Independent Test**: Modify JSON config files, restart application, verify settings take effect

### Tests for User Story 3

- [ ] T032 [P] [US3] Unit test for Configuration entity validation in tests/unit/test_configuration.cpp
- [ ] T033 [P] [US3] Unit test for ConfigurationManager loading in tests/unit/test_configuration_manager.cpp
- [ ] T034 [P] [US3] Integration test for config file changes in tests/integration/test_config_reload.cpp

### Implementation for User Story 3

- [x] T035 [P] [US3] Create Configuration entity in include/config/configuration.h
- [ ] T036 [P] [US3] Create ConfigurationManager class in include/config/configuration_manager.h
- [x] T037 [US3] Implement Configuration with JSON validation in src/config/configuration.cpp
- [ ] T038 [US3] Implement ConfigurationManager with file loading in src/config/configuration_manager.cpp
- [ ] T039 [US3] Integrate configuration management into existing components
- [x] T040 [US3] Create example configuration files in config/

**Checkpoint**: All user stories should now be independently functional

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [ ] T041 [P] Create comprehensive documentation in docs/
- [ ] T042 [P] Add connection monitoring and health checks
- [ ] T043 [P] Implement graceful shutdown handling
- [ ] T044 [P] Performance optimization for high-frequency readings
- [ ] T045 [P] Security hardening for production deployment
- [ ] T046 Run quickstart.md validation and update as needed

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - User stories can then proceed in parallel (if staffed)
  - Or sequentially in priority order (P1 → P2 → P3)
- **Polish (Final Phase)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories
- **User Story 2 (P2)**: Can start after Foundational (Phase 2) - Extends US1 but should be independently testable
- **User Story 3 (P3)**: Can start after Foundational (Phase 2) - Integrates with US1/US2 but should be independently testable

### Within Each User Story

- Tests MUST be written and FAIL before implementation
- Models before services
- Services before main application integration
- Core implementation before integration
- Story complete before moving to next priority

### Parallel Opportunities

- All Setup tasks marked [P] can run in parallel
- All Foundational tasks marked [P] can run in parallel (within Phase 2)
- Once Foundational phase completes, all user stories can start in parallel (if team capacity allows)
- All tests for a user story marked [P] can run in parallel
- Models within a story marked [P] can run in parallel
- Different user stories can be worked on in parallel by different team members

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Test User Story 1 independently
5. Deploy/demo if ready

### Incremental Delivery

1. Complete Setup + Foundational → Foundation ready
2. Add User Story 1 → Test independently → Deploy/Demo (MVP!)
3. Add User Story 2 → Test independently → Deploy/Demo
4. Add User Story 3 → Test independently → Deploy/Demo
5. Each story adds value without breaking previous stories

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together
2. Once Foundational is done:
   - Developer A: User Story 1
   - Developer B: User Story 2
   - Developer C: User Story 3
3. Stories complete and integrate independently

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- Verify tests fail before implementing
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- Constitution compliance: C++17, static Paho linking, test-first development, connection resilience
- Message discarding during disconnection (justified constitution violation for PoC simplicity)