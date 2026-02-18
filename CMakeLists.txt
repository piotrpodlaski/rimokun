cmake_minimum_required(VERSION 3.28)
project(rimokun)

set(CMAKE_CXX_STANDARD 23)

include(CTest)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    # Allow older third-party CMakeLists (e.g. yaml-cpp) under CMake 4+.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif ()

option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
option(ENABLE_TIMING_METRICS
       "Enable runtime timing metrics instrumentation and periodic reports" OFF)
option(ENABLE_PROFILING
       "Enable profiler-friendly instrumentation (debug symbols + frame pointers)" OFF)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG master
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
find_package(nlohmann_json CONFIG QUIET)
if (NOT nlohmann_json_FOUND)
    FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.3
    )
endif ()
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
if (NOT nlohmann_json_FOUND)
    FetchContent_MakeAvailable(nlohmann_json)
endif ()
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

if (ENABLE_TIMING_METRICS)
    add_compile_definitions(RIMOKUN_TIMING_ENABLED=1)
else ()
    add_compile_definitions(RIMOKUN_TIMING_ENABLED=0)
endif ()

if (ENABLE_PROFILING)
    message(STATUS "Profiler-friendly build flags enabled for all configurations")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-g -fno-omit-frame-pointer)
    elseif (MSVC)
        add_compile_options(/Zi /Oy-)
    else ()
        message(WARNING "ENABLE_PROFILING is set, but no compiler-specific flags are configured for ${CMAKE_CXX_COMPILER_ID}.")
    endif ()
endif ()


add_subdirectory(Utilities)
add_subdirectory(GUI)
add_subdirectory(Server)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
