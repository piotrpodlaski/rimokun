cmake_minimum_required(VERSION 3.28)
project(rimokun)

set(CMAKE_CXX_STANDARD 23)

include(CTest)

option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

include(FetchContent)
FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
)
FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG v0.9.7
)

FetchContent_Declare(
        cppzmq
        GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
        GIT_TAG v4.10.0
)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
)

FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
        GIT_TAG v3.2
)
option(CPPZMQ_BUILD_TESTS OFF)
if (BUILD_TESTING)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(yaml-cpp magic_enum cppzmq spdlog argparse)
if (BUILD_TESTING)
    FetchContent_MakeAvailable(googletest)
endif()

#Handle spdlog debug level:
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
else()
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
endif()
add_compile_definitions(SPDLOG_COMPILED_LIB)

if (ENABLE_COVERAGE)
    if (NOT BUILD_TESTING)
        message(FATAL_ERROR "ENABLE_COVERAGE requires BUILD_TESTING=ON.")
    endif ()
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "ENABLE_COVERAGE is supported only with GCC/Clang.")
    endif ()

    message(STATUS "Code coverage instrumentation enabled")
    add_compile_options(-O0 -g --coverage)
    add_link_options(--coverage)
endif ()


add_subdirectory(Utilities)
add_subdirectory(GUI)
add_subdirectory(Server)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
