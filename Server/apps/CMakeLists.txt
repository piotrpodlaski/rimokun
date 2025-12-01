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
