add_executable(rimoServer rimoServer.cpp)
target_link_libraries(rimoServer PRIVATE rimoSrvlib utilities yaml-cpp)
