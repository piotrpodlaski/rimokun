add_executable(utilities_unit_tests
        utilities/ConfigTests.cpp
        utilities/YamlExtensionsTests.cpp
        utilities/VMotorStatsTests.cpp
)

target_link_libraries(utilities_unit_tests
        PRIVATE
        utilities
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(utilities_unit_tests)

add_executable(server_unit_tests
        server/ControlLoopRunnerTests.cpp
        server/MachineControllerTests.cpp
        server/RobotControlPolicyTests.cpp
        server/MachineCommandProcessorTests.cpp
        server/MachineCommandServerTests.cpp
        server/MachineComponentServiceTests.cpp
        server/MachineStatusBuilderTests.cpp
        server/CommandQueueTests.cpp
        server/MachineLoopTests.cpp
        server/MachineRuntimeTests.cpp
        server/MachineCommandTests.cpp
        server/MachineMappingTests.cpp
        server/ArKd2DiagnosticsTests.cpp
        server/ArKd2RegisterMapTests.cpp
        server/ControlPanelCommFactoryTests.cpp
        server/SerialControlPanelCommTests.cpp
)

target_include_directories(server_unit_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/Server/include
        ${CMAKE_SOURCE_DIR}/Utilities/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(server_unit_tests
        PRIVATE
        utilities
        rimoSrvlib
        GTest::gtest_main
)

gtest_discover_tests(server_unit_tests)

add_executable(motor_unit_tests
        server/MotorTests.cpp
        server/MotorControlTests.cpp
        server/ModbusClientTests.cpp
        server/fakes/FakeModbus.cpp
)

target_include_directories(motor_unit_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/Server/include
        ${CMAKE_SOURCE_DIR}/Utilities/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(motor_unit_tests
        PRIVATE
        utilities
        rimoSrvlib
        GTest::gtest_main
)

gtest_discover_tests(motor_unit_tests)

if (ENABLE_COVERAGE)
    string(REPLACE "." ";" CXX_VERSION_PARTS "${CMAKE_CXX_COMPILER_VERSION}")
    list(GET CXX_VERSION_PARTS 0 CXX_VERSION_MAJOR)
    find_program(GCOV_BIN
            NAMES gcov-${CXX_VERSION_MAJOR} gcov
    )
    find_program(LCOV_BIN lcov)
    find_program(GENHTML_BIN genhtml)
    if (LCOV_BIN AND GENHTML_BIN AND GCOV_BIN)
        set(COVERAGE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/coverage)
        set(COVERAGE_INFO ${COVERAGE_OUTPUT_DIR}/coverage.info)

        add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}
                COMMAND ${LCOV_BIN} --directory ${CMAKE_BINARY_DIR} --zerocounters
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND ${LCOV_BIN}
                --gcov-tool ${GCOV_BIN}
                --ignore-errors mismatch,gcov
                --rc geninfo_unexecuted_blocks=1
                --directory ${CMAKE_BINARY_DIR}
                --capture
                --output-file ${COVERAGE_INFO}
                COMMAND ${LCOV_BIN} --remove ${COVERAGE_INFO}
                --ignore-errors unused
                /usr/*
                ${CMAKE_SOURCE_DIR}/tests/*
                ${CMAKE_BINARY_DIR}/_deps/*
                ${CMAKE_BINARY_DIR}/*
                --output-file ${COVERAGE_INFO}
                COMMAND ${GENHTML_BIN} ${COVERAGE_INFO} --output-directory ${COVERAGE_OUTPUT_DIR}/html
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running tests and generating coverage report in ${COVERAGE_OUTPUT_DIR}/html"
                VERBATIM
        )
    else ()
        message(WARNING "ENABLE_COVERAGE is ON but gcov/lcov/genhtml were not found; 'coverage' target will not be available.")
    endif ()
endif ()
