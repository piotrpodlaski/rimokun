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
        server/MachineCommandProcessorTests.cpp
        server/MachineComponentServiceTests.cpp
        server/MachineStatusBuilderTests.cpp
        server/CommandQueueTests.cpp
        server/MachineLoopTests.cpp
        server/MachineCommandTests.cpp
        server/MachineMappingTests.cpp
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
