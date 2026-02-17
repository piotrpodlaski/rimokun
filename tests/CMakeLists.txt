add_subdirectory(unit)
add_subdirectory(integration)

add_custom_target(test-unit
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L unit
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running unit tests"
)

add_custom_target(test-integration
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L integration
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running integration tests"
)
