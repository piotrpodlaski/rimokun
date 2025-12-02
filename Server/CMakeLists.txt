file(GLOB rimoServer_SOURCES CONFIGURE_DEPENDS src/*.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMODBUS REQUIRED libmodbus)

message(${rimoServer_SOURCES})
add_library(rimoSrvlib STATIC ${rimoServer_SOURCES})

target_include_directories(rimoSrvlib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${LIBMODBUS_INCLUDE_DIRS}
)

target_link_libraries(rimoSrvlib PUBLIC utilities ${LIBMODBUS_LIBRARIES})

add_subdirectory(apps)
