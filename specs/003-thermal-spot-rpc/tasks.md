# Tasks: Thermal Camera Spot Measurement RPC Control

**Input**: Design documents from `/specs/003-thermal-spot-rpc/`
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
- Paths assume extension of existing C++ thermal camera project structure

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: C++ project extension for thermal spot RPC functionality

- [x] T001 Create RPC module directories src/thingsboard/rpc/ and include/thingsboard/rpc/
- [x] T002 Create thermal management directories src/thermal/spot_manager/ and include/thermal/spot_manager/
- [x] T003 [P] Create temperature source directories src/thermal/temperature_source/ and include/thermal/temperature_source/
- [x] T004 [P] Update CMakeLists.txt to include new thermal RPC modules and maintain C++17 compliance
- [x] T005 [P] Create test directories for new modules: tests/unit/thermal/rpc/ and tests/integration/thermal/

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure for thermal spot management and RPC that MUST be complete before ANY user story implementation

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [x] T006 Analyze existing MeasurementSpot class integration requirements in src/thermal/measurement_spot.cpp
- [x] T007 Analyze existing paho_device.cpp RPC subscription integration points and callback structure
- [x] T008 Document integration approach for extending existing thermal components with RPC capabilities
- [x] T009 Implement TemperatureDataSource interface in include/thermal/temperature_source/temperature_data_source.h
- [x] T010 [P] Implement CoordinateBasedTemperatureSource class in src/thermal/temperature_source/coordinate_based_source.cpp
- [x] T011 [P] Create ThermalSpotManager class header with spot lifecycle management in include/thermal/spot_manager/thermal_spot_manager.h
- [x] T012 Create RPCCommand and RPCResponse data structures in include/thingsboard/rpc/rpc_types.h
- [x] T013 [P] Implement JSON spot persistence manager in src/thermal/spot_manager/spot_persistence.cpp
- [x] T014 Extend existing paho_device.cpp to subscribe to RPC topic v1/devices/me/rpc/request/+ 
- [x] T015 [P] Create RPC message parser and validator in src/thingsboard/rpc/rpc_parser.cpp
- [x] T016 Implement RPC command timeout handling (5000ms) with proper error responses in src/thingsboard/rpc/rpc_timeout_manager.cpp
- [x] T017 [P] Implement sequential RPC command processing queue to prevent race conditions in src/thingsboard/rpc/rpc_command_queue.cpp
- [x] T018 [P] Add maximum spots enforcement (FR-014) validation in ThermalSpotManager class
- [x] T023 [US5] Implement TemperatureDataSource base class in src/thermal/temperature_source/temperature_data_source.cpp
- [x] T024 [US5] Implement coordinate-based temperature algorithm with ±0.5°C variation in src/thermal/temperature_source/coordinate_based_source.cpp
- [x] T025 [US5] Implement ThermalSpotManager with enableSpot/disableSpot/moveSpot methods in src/thermal/spot_manager/thermal_spot_manager.cpp

**Checkpoint**: Foundation ready - spot management and RPC infrastructure available for user story implementation

---

## Phase 3: User Story 5 - Expand Thermal Camera Simulator (Priority: P0) 🎯 Foundation

**Goal**: Extend existing basic thermal camera simulator to support dynamic spot management with modular architecture

**Independent Test**: Verify simulator can dynamically enable/disable spots, move spots, persist configurations, and provide clean temperature data source interface

### Tests for User Story 5

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [ ] T019 [P] [US5] Unit test for TemperatureDataSource interface in tests/unit/thermal/test_temperature_data_source.cpp
- [ ] T020 [P] [US5] Unit test for CoordinateBasedTemperatureSource algorithm in tests/unit/thermal/test_coordinate_based_source.cpp
- [ ] T021 [P] [US5] Unit test for ThermalSpotManager spot lifecycle in tests/unit/thermal/test_thermal_spot_manager.cpp
- [ ] T022 [P] [US5] Unit test for spot persistence functionality in tests/unit/thermal/test_spot_persistence.cpp

### Implementation for User Story 5

- [ ] T023 [US5] Implement TemperatureDataSource base class in src/thermal/temperature_source/temperature_data_source.cpp
- [ ] T024 [US5] Implement coordinate-based temperature algorithm with ±0.5°C variation in src/thermal/temperature_source/coordinate_based_source.cpp
- [ ] T025 [US5] Implement ThermalSpotManager with enableSpot/disableSpot/moveSpot methods in src/thermal/spot_manager/thermal_spot_manager.cpp
- [x] T026 [US5] Implement JSON spot persistence with graceful corruption handling in src/thermal/spot_manager/spot_persistence.cpp
- [ ] T027 [US5] Extend existing MeasurementSpot class to support RPC-driven state management
- [ ] T028 [US5] Add modular temperature data source factory pattern for future remote integration
- [ ] T029 [US5] Integration test for complete simulator extension in tests/integration/thermal/test_simulator_extension.cpp

**Checkpoint**: Thermal simulator foundation ready - can create, move, delete spots programmatically with temperature simulation

---

## Phase 4: User Story 1 - Create Spot Measurement (Priority: P1) 🎯 MVP

**Goal**: Enable operators to create new thermal measurement spots via ThingsBoard RPC commands

**Independent Test**: Send createSpotMeasurement RPC command via ThingsBoard and verify thermal camera creates spot and responds with success confirmation

### Tests for User Story 1

- [ ] T030 [P] [US1] Unit test for createSpotMeasurement RPC handler in tests/unit/thingsboard/rpc/test_create_spot_handler.cpp
- [ ] T031 [P] [US1] Unit test for RPC parameter validation in tests/unit/thingsboard/rpc/test_rpc_validation.cpp
- [ ] T032 [P] [US1] Integration test for complete createSpotMeasurement flow in tests/integration/thingsboard/test_create_spot_rpc.cpp

### Implementation for User Story 1

- [ ] T033 [P] [US1] Create ThermalRPCHandler class header in include/thingsboard/rpc/thermal_rpc_handler.h
- [ ] T034 [US1] Implement createSpotMeasurement RPC handler with parameter validation in src/thingsboard/rpc/thermal_rpc_handler.cpp
- [ ] T035 [US1] Implement RPC response formatting for success and error cases in src/thingsboard/rpc/rpc_response_formatter.cpp
- [ ] T036 [US1] Add createSpotMeasurement message routing in existing paho_device.cpp RPC callback
- [ ] T037 [US1] Implement coordinate validation (0-319, 0-239) and spotId validation (1-5) in RPC handler
- [ ] T038 [US1] Add error handling for duplicate spot creation and maximum spots reached scenarios
- [ ] T039 [US1] Integrate ThermalSpotManager with RPC handler for spot creation and temperature reading

**Checkpoint**: At this point, createSpotMeasurement RPC should be fully functional and independently testable

---

## Phase 5: User Story 4 - List All Active Spots (Priority: P2)

**Goal**: Enable operators to view all currently active measurement spots and their status via RPC

**Independent Test**: Send listSpotMeasurements RPC command and verify response contains accurate information about all active spots

### Tests for User Story 4

- [ ] T040 [P] [US4] Unit test for listSpotMeasurements RPC handler in tests/unit/thingsboard/rpc/test_list_spots_handler.cpp
- [ ] T041 [P] [US4] Integration test for listSpotMeasurements with multiple spots in tests/integration/thingsboard/test_list_spots_rpc.cpp

### Implementation for User Story 4

- [ ] T042 [P] [US4] Implement listSpotMeasurements RPC handler in src/thingsboard/rpc/thermal_rpc_handler.cpp
- [ ] T043 [US4] Add spot enumeration and status reporting functionality to ThermalSpotManager
- [ ] T044 [US4] Implement JSON response formatting for spots array with coordinates and temperatures
- [ ] T045 [US4] Add listSpotMeasurements message routing in paho_device.cpp RPC callback
- [ ] T046 [US4] Handle empty spots scenario with proper JSON response (totalSpots: 0)

**Checkpoint**: At this point, User Stories 1 AND 4 should both work independently

---

## Phase 6: User Story 2 - Move Existing Spot Measurement (Priority: P2)

**Goal**: Enable operators to relocate existing measurement spots to new coordinates via RPC

**Independent Test**: Create a spot, then send moveSpotMeasurement RPC command with new coordinates and verify spot moves with temperature at new location

### Tests for User Story 2

- [ ] T047 [P] [US2] Unit test for moveSpotMeasurement RPC handler in tests/unit/thingsboard/rpc/test_move_spot_handler.cpp
- [ ] T048 [P] [US2] Integration test for spot movement workflow in tests/integration/thingsboard/test_move_spot_rpc.cpp

### Implementation for User Story 2

- [ ] T049 [P] [US2] Implement moveSpotMeasurement RPC handler in src/thingsboard/rpc/thermal_rpc_handler.cpp
- [ ] T050 [US2] Implement spot existence validation before movement in RPC handler
- [ ] T051 [US2] Add coordinate validation for new position in moveSpotMeasurement handler
- [ ] T052 [US2] Update ThermalSpotManager to support spot coordinate updates
- [ ] T053 [US2] Add moveSpotMeasurement message routing in paho_device.cpp RPC callback
- [ ] T054 [US2] Implement error handling for invalid moves (non-existent spot, invalid coordinates)

**Checkpoint**: At this point, User Stories 1, 2, AND 4 should all work independently

---

## Phase 7: User Story 3 - Delete Spot Measurement (Priority: P3)

**Goal**: Enable operators to remove measurement spots that are no longer needed via RPC

**Independent Test**: Create a spot, delete it via deleteSpotMeasurement RPC, and verify it no longer appears in spot listings

### Tests for User Story 3

- [ ] T055 [P] [US3] Unit test for deleteSpotMeasurement RPC handler in tests/unit/thingsboard/rpc/test_delete_spot_handler.cpp
- [ ] T056 [P] [US3] Integration test for complete deleteSpotMeasurement flow in tests/integration/thingsboard/test_delete_spot_rpc.cpp

### Implementation for User Story 3

- [ ] T057 [P] [US3] Implement deleteSpotMeasurement RPC handler in src/thingsboard/rpc/thermal_rpc_handler.cpp
- [ ] T058 [US3] Implement spot existence validation before deletion in RPC handler
- [ ] T059 [US3] Add deleteSpotMeasurement message routing to existing paho_device.cpp RPC callback
- [ ] T060 [US3] Add error handling for non-existent spot deletion attempts
- [ ] T061 [US3] Implement cleanup of spot data and temperature simulation on deletion
- [ ] T062 [US3] Update ThermalSpotManager to remove spots and clean up resources

**Checkpoint**: All core user stories should now be independently functional

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Improvements and validations that affect multiple user stories

- [ ] T057 [P] Add comprehensive error logging for all RPC operations in thermal RPC handler
- [ ] T058 [P] Implement RPC command timeout handling (5000ms) with proper error responses
- [ ] T059 [P] Add performance monitoring for temperature reading response times (SC-006: <500ms)
- [ ] T066 [P] Implement sequential RPC command processing queue to prevent race conditions
- [ ] T067 [P] Add memory management verification (no dynamic allocation during message processing)
- [ ] T068 [P] Create comprehensive integration test covering all RPC commands in sequence
- [ ] T069 Run quickstart.md test scenarios to validate complete implementation
- [ ] T070 [P] Update thermal_config.json schema to include RPC configuration section
- [ ] T071 [P] Add graceful shutdown handling for active spots and persistence

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Story 5 (Phase 3)**: Depends on Foundational - Foundation for all other stories
- **User Stories 1,2,3,4 (Phases 4-7)**: All depend on User Story 5 completion
  - User stories can proceed in parallel (if staffed) or sequentially by priority
- **Polish (Phase 8)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 5 (P0)**: Foundation - MUST complete before any other user story
- **User Story 1 (P1)**: MVP capability - depends on US5, no dependencies on other stories
- **User Story 4 (P2)**: List functionality - depends on US5 and US1 (to have spots to list)
- **User Story 2 (P2)**: Move functionality - depends on US5 and US1 (to have spots to move)  
- **User Story 3 (P3)**: Delete functionality - depends on US5 and US1 (to have spots to delete)

### Recommended Execution Order

1. **MVP Sequence**: US5 → US1 → US4 (Foundation + Create + List = basic functionality)
2. **Full Feature Sequence**: US5 → US1 → US4 → US2 → US3 (priority order)
3. **Parallel Option**: After US5, can run US1 and US4 in parallel, then US2 and US3 in parallel

### Within Each User Story

- Tests MUST be written and FAIL before implementation (TDD approach)
- Unit tests before integration tests
- Handler implementation before integration
- Core functionality before error handling
- Story complete and tested before moving to next priority

### Parallel Opportunities

- All Setup tasks marked [P] can run in parallel
- All Foundational tasks marked [P] can run in parallel (within Phase 2)
- After US5 completes, US1 and US4 can start in parallel (if team capacity allows)
- After US1 completes, US2 and US3 can run in parallel
- All tests for a user story marked [P] can run in parallel
- Implementation tasks within a story marked [P] can run in parallel

---

## Parallel Example: User Story 1

```bash
# Launch all tests for User Story 1 together:
Task: "Unit test for createSpotMeasurement RPC handler in tests/unit/thingsboard/rpc/test_create_spot_handler.cpp"
Task: "Unit test for RPC parameter validation in tests/unit/thingsboard/rpc/test_rpc_validation.cpp"
Task: "Integration test for complete createSpotMeasurement flow in tests/integration/thingsboard/test_create_spot_rpc.cpp"

# Launch parallel implementation tasks for User Story 1:
Task: "Create ThermalRPCHandler class header in include/thingsboard/rpc/thermal_rpc_handler.h"
Task: "Implement RPC response formatting for success and error cases in src/thingsboard/rpc/rpc_response_formatter.cpp"
```

---

## Implementation Strategy

### MVP First (User Stories 5 + 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 5 (Foundation)
4. Complete Phase 4: User Story 1 (Create spots)
5. **STOP and VALIDATE**: Test creation workflow independently
6. Deploy/demo basic spot creation capability

### Recommended Delivery Sequence

1. **Foundation Ready**: Setup + Foundational + US5 → Simulator can manage spots programmatically
2. **MVP Ready**: + US1 → Operators can create spots via RPC (immediate value!)
3. **Operational Ready**: + US4 → Operators can list spots (visibility and management)
4. **Full Control Ready**: + US2 + US3 → Complete spot lifecycle management

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational + US5 together (foundation)
2. Once US5 is done:
   - Developer A: User Story 1 (create - highest priority)
   - Developer B: User Story 4 (list - complements create)
3. After US1 complete:
   - Developer A: User Story 2 (move)
   - Developer B: User Story 3 (delete)
4. Stories integrate independently through ThermalSpotManager

---

## Success Criteria Validation

### Performance Requirements

- **SC-001**: Create spot response < 2 seconds - Validate in T026 integration test
- **SC-003**: Move spot response < 1 second - Validate in T042 integration test  
- **SC-006**: Temperature reading < 500ms - Validate in T059 performance monitoring
- **SC-007**: 5 concurrent spots - Validate in T068 comprehensive integration test

### Functional Requirements

- **SC-002**: 100% valid command processing - All unit tests must pass
- **SC-005**: 100% invalid coordinate prevention - Validate in T025 validation test

### Implementation Checkpoints

- After T023: Simulator foundation complete
- After T033: Basic spot creation functional
- After T040: Spot listing and visibility complete
- After T048: Spot movement functional
- After T056: Complete spot lifecycle management
- After T069: End-to-end workflow validation

---

## Notes

- [P] tasks = different files, no dependencies, can run parallel
- [Story] label maps task to specific user story for traceability  
- Each user story should be independently completable and testable
- US5 is foundational and must complete before other user stories
- All RPC handlers extend existing paho_device.cpp MQTT infrastructure
- Temperature simulation reuses existing MeasurementSpot infrastructure
- 80%+ test coverage achieved through comprehensive unit and integration tests
- Follow quickstart.md for end-to-end validation scenarios