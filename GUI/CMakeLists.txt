find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

qt_standard_project_setup()

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/forms)

file(GLOB_RECURSE gui_sources "src/*.cpp")
file(GLOB_RECURSE gui_headers "include/*.hpp")
file(GLOB_RECURSE gui_forms "forms/*.ui")


qt_add_executable(rimokunControl
        ${gui_sources}
        ${gui_headers}
        ${gui_forms}
        rimokunControl.cpp

)

qt_add_resources(rimokunControl "images"
        PREFIX "/"
        FILES resources/rimoKunLogo.png
        resources/KEKLogo.png
)

target_include_directories(rimokunControl
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(rimokunControl PRIVATE
        Qt6::Widgets
        magic_enum
        utilities
        cppzmq
        yaml-cpp::yaml-cpp
        spdlog
        argparse
)
