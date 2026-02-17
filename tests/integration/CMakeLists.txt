add_executable(server_integration_tests
        server/MachineRuntimeIntegrationTests.cpp
)

target_include_directories(server_integration_tests
        PRIVATE
        ${CMAKE_SOURCE_DIR}/Server/include
        ${CMAKE_SOURCE_DIR}/Utilities/include
        ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(server_integration_tests
        PRIVATE
        utilities
        rimoSrvlib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(server_integration_tests PROPERTIES LABELS "integration")
