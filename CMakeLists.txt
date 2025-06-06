cmake_minimum_required(VERSION 3.31)
project(rimokun)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
)
FetchContent_MakeAvailable(yaml-cpp)


add_subdirectory(GUI)