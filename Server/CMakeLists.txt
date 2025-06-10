file(GLOB rimoServer_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(rimoSrvlib STATIC ${rimoServer_SOURCES})

target_include_directories(rimoSrvlib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(apps)
