cmake_minimum_required(VERSION 3.10)
project(at_modem_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(PROJECT_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

add_executable(at_modem_server
    src/main.cpp
    src/command_matcher.cpp
    src/expect_table.cpp
    src/serial_port.cpp
    src/modem_server.cpp
)

target_compile_options(at_modem_server PRIVATE ${PROJECT_WARNINGS})
target_include_directories(at_modem_server PRIVATE ${PROJECT_SOURCE_DIR}/src)

enable_testing()

add_executable(matcher_tests
    tests/matcher_tests.cpp
    src/command_matcher.cpp
)

target_compile_options(matcher_tests PRIVATE ${PROJECT_WARNINGS})
target_include_directories(matcher_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_executable(integration_test
    tests/integration_tests.cpp
)

target_compile_options(integration_test PRIVATE ${PROJECT_WARNINGS})
add_dependencies(integration_test at_modem_server)

add_test(NAME matcher_tests COMMAND matcher_tests)
add_test(NAME integration_test COMMAND integration_test)

set_tests_properties(integration_test PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "AT_MODEM_SERVER=$<TARGET_FILE:at_modem_server>;AT_MODEM_CONFIG=${PROJECT_SOURCE_DIR}/config/expect.txt"
)
