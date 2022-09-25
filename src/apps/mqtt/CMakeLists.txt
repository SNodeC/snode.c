cmake_minimum_required(VERSION 3.0)

set(MQTTSERVER_CPP
    mqttbroker.cpp broker/Broker.cpp broker/SocketContext.cpp
    broker/SocketContextFactory.cpp broker/SubscriberTree.cpp
    broker/TopicTree.cpp
)
set(MQTTSERVER_H
    broker/Broker.h broker/SocketContext.h broker/SocketContextFactory.h
    broker/SubscriberTree.h broker/TopicTree.h
)

add_executable(mqttbroker ${MQTTSERVER_CPP} ${MQTTSERVER_H})

target_link_libraries(
    mqttbroker PUBLIC snodec::mqtt snodec::net-in-stream-legacy
)

set_target_properties(mqttbroker PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

install(TARGETS mqttbroker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
