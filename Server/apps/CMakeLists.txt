add_executable(rimoServerDummy rimoServerDummy.cpp)
target_link_libraries(rimoServerDummy PRIVATE  rimoSrvlib)

add_executable(rimoServer rimoServer.cpp)
target_link_libraries(rimoServer PRIVATE  rimoSrvlib)

add_executable(modbusTestRTU modbusTestRTU.cpp)
target_link_libraries(modbusTestRTU  PUBLIC  rimoSrvlib)

add_executable(modbusTestTCP modbusTestTCP.cpp)
target_link_libraries(modbusTestTCP  PRIVATE  rimoSrvlib)

add_executable(modbusRtuOverTcpExample modbusRtuOverTcpExample.cpp)
target_link_libraries(modbusRtuOverTcpExample PRIVATE rimoSrvlib)

add_executable(motorControlDemo motorControlDemo.cpp)
target_link_libraries(motorControlDemo PRIVATE rimoSrvlib)
