cmake_minimum_required(VERSION 3.16)

project(thermal-mqtt-client 
    VERSION 1.0.0
    DESCRIPTION "Thermal Camera MQTT Client for ThingsBoard"
    LANGUAGES CXX)

# C++17 requirement from constitution
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags from constitution: -Wall -Wextra -Werror
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Include FetchContent for dependencies
include(FetchContent)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# JSON library for configuration
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# Find system-installed Paho MQTT C library
find_library(PAHO_MQTT3A_LIB paho-mqtt3a PATHS /opt/homebrew/lib)
find_library(PAHO_MQTT3AS_LIB paho-mqtt3as PATHS /opt/homebrew/lib)
find_path(PAHO_MQTT_INCLUDE_DIR MQTTAsync.h PATHS /opt/homebrew/include)

if(NOT PAHO_MQTT3A_LIB OR NOT PAHO_MQTT3AS_LIB OR NOT PAHO_MQTT_INCLUDE_DIR)
    message(WARNING "Paho MQTT C library not found, using mock implementation")
    set(USE_REAL_MQTT OFF)
else()
    message(STATUS "Found Paho MQTT C library: ${PAHO_MQTT3AS_LIB}")
    message(STATUS "Paho MQTT include dir: ${PAHO_MQTT_INCLUDE_DIR}")
    set(USE_REAL_MQTT ON)
    include_directories(${PAHO_MQTT_INCLUDE_DIR})
endif()

# Source files
set(THERMAL_SOURCES
    src/thermal/temperature_reading.cpp
    src/thermal/measurement_spot.cpp
)

# Choose MQTT implementation based on availability
if(USE_REAL_MQTT)
    set(MQTT_SOURCES
        src/mqtt/paho_c_client.cpp  # Real Paho MQTT C implementation
    )
    set(THINGSBOARD_SOURCES
        src/thingsboard/paho_device.cpp  # Real ThingsBoard device implementation
    )
else()
    set(MQTT_SOURCES
        src/mqtt/mock_client.cpp  # Mock implementation for testing
    )
    set(THINGSBOARD_SOURCES
        src/thingsboard/mock_device.cpp  # Mock implementation for testing
    )
endif()

set(CONFIG_SOURCES
    src/config/configuration.cpp
)

set(COMMON_SOURCES
    src/common/logger.cpp
    src/common/error_handler.cpp
)

set(LIB_SOURCES
    ${THERMAL_SOURCES}
    ${MQTT_SOURCES}
    ${THINGSBOARD_SOURCES}
    ${CONFIG_SOURCES}
    ${COMMON_SOURCES}
)

# Create a library for core functionality
add_library(thermal-core STATIC ${LIB_SOURCES})

# Link libraries based on MQTT implementation
if(USE_REAL_MQTT)
    target_link_libraries(thermal-core 
        PRIVATE 
        nlohmann_json::nlohmann_json
        ${PAHO_MQTT3AS_LIB}
    )
else()
    target_link_libraries(thermal-core 
        PRIVATE 
        nlohmann_json::nlohmann_json
    )
endif()

# Create the main MQTT application (User Story 1)
if(USE_REAL_MQTT)
    add_executable(thermal-mqtt-client
        src/main_paho.cpp
    )
else()
    add_executable(thermal-mqtt-client
        src/main.cpp
    )
endif()

# Link the core library
target_link_libraries(thermal-mqtt-client
    PRIVATE
    thermal-core
    nlohmann_json::nlohmann_json
)

# Create the continuous telemetry application (User Story 2)
add_executable(thermal-continuous-client
    src/main_continuous.cpp
)

# Link the core library
target_link_libraries(thermal-continuous-client
    PRIVATE
    thermal-core
    nlohmann_json::nlohmann_json
)

# Create the real Paho MQTT application (T002 compliance)
add_executable(thermal-paho-client
    src/main_paho.cpp
)

# Link the core library with Paho MQTT
target_link_libraries(thermal-paho-client
    PRIVATE
    thermal-core
    nlohmann_json::nlohmann_json
    paho-mqttpp3-static
    paho-mqtt3as-static
)

# Keep the config test executable
add_executable(thermal-test-config
    src/main_config_test.cpp
)

target_link_libraries(thermal-test-config
    PRIVATE
    thermal-core
    nlohmann_json::nlohmann_json
)

# Testing setup (Google Test)
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    
    # Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
    
    # Test sources
    set(TEST_SOURCES
        tests/unit/test_temperature_reading.cpp
        tests/unit/test_measurement_spot.cpp
        tests/integration/test_multi_spot.cpp
    )
    
    add_executable(thermal-tests ${TEST_SOURCES})
    
    target_link_libraries(thermal-tests
        PRIVATE
        gtest_main
        gmock_main
        thermal-core
        nlohmann_json::nlohmann_json
    )
    
    # Discover tests
    include(GoogleTest)
    gtest_discover_tests(thermal-tests)
endif()

# Create example configuration file
configure_file(
    ${CMAKE_SOURCE_DIR}/config/config.example.json
    ${CMAKE_BINARY_DIR}/config.json
    COPYONLY
)