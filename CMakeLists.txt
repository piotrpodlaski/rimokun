cmake_minimum_required(VERSION 3.28)
project(rimokun)

set(CMAKE_CXX_STANDARD 23)

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

FetchContent_MakeAvailable(yaml-cpp)
FetchContent_MakeAvailable(magic_enum)


add_subdirectory(Utilities)
add_subdirectory(GUI)
