add_executable(rimoServer RimoServer.cpp)
target_link_libraries(
        rimoServer
        PRIVATE
            rimoSrvlib
            utilities
            yaml-cpp
            magic_enum
            spdlog
            argparse
            ${LIBMODBUS_LIBRARIES}
)

add_executable(modbusTestRTU modbusTestRTU.cpp)
target_link_libraries(
        modbusTestRTU
        PRIVATE
        rimoSrvlib
        utilities
        yaml-cpp
        magic_enum
        spdlog
        argparse
        ${LIBMODBUS_LIBRARIES}
)

add_executable(modbusTestTCP modbusTestTCP.cpp)
target_link_libraries(
        modbusTestTCP
        PRIVATE
        rimoSrvlib
        utilities
        yaml-cpp
        magic_enum
        spdlog
        argparse
        ${LIBMODBUS_LIBRARIES}
)