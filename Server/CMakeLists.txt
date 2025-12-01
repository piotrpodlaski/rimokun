file(GLOB rimoServer_SOURCES CONFIGURE_DEPENDS src/*.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBMODBUS REQUIRED libmodbus)


add_library(rimoSrvlib STATIC ${rimoServer_SOURCES})

target_include_directories(rimoSrvlib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${LIBMODBUS_INCLUDE_DIRS}
)

add_subdirectory(apps)
