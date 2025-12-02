add_executable(rimoServer RimoServer.cpp)
target_link_libraries(rimoServer PRIVATE  rimoSrvlib)

add_executable(modbusTestRTU modbusTestRTU.cpp)
target_link_libraries(modbusTestRTU  PUBLIC  rimoSrvlib)

add_executable(modbusTestTCP modbusTestTCP.cpp)
target_link_libraries(modbusTestTCP  PRIVATE  rimoSrvlib)
