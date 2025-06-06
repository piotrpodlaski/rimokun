find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

qt_standard_project_setup()

set(CMAKE_AUTOUIC_SEARCH_PATHS forms/)

file(GLOB_RECURSE gui_sources "src/*.cpp")
file(GLOB_RECURSE gui_headers "include/*.hpp")


qt_add_executable(rimokunControl
        ${gui_sources}
        ${gui_headers}
        rimokunControl.cpp

)

target_include_directories(rimokunControl
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rimokunControl PRIVATE Qt6::Widgets PUBLIC yaml-cpp::yaml-cpp)