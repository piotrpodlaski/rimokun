file(GLOB_RECURSE utils_sources "src/*.cpp")


add_library(utilities STATIC ${utils_sources})

target_include_directories(utilities
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(utilities PUBLIC magic_enum cppzmq spdlog argparse yaml-cpp)