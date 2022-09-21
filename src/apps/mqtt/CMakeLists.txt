cmake_minimum_required(VERSION 3.0)

set(MQTTSERVER_CPP mqttserver.cpp server/SocketContext.cpp
                   server/SocketContextFactory.cpp
)
set(MQTTSERVER_H server/SocketContext.h server/SocketContextFactory.h)

add_executable(mqttserver ${MQTTSERVER_CPP} ${MQTTSERVER_H})

set_target_properties(mqttserver PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

target_link_libraries(
    mqttserver PUBLIC snodec::mqtt snodec::net-in-stream-legacy
)

install(TARGETS mqttserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
