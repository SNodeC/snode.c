cmake_minimum_required(VERSION 3.0)

set(MQTTSERVER_CPP
    mqttserver.cpp server/Broker.cpp server/SocketContext.cpp
    server/SocketContextFactory.cpp server/SubscriberTree.cpp
    server/TopicTree.cpp
)
set(MQTTSERVER_H
    server/Broker.h server/SocketContext.h server/SocketContextFactory.h
    server/SubscriberTree.h server/TopicTree.h
)

add_executable(mqttserver ${MQTTSERVER_CPP} ${MQTTSERVER_H})

target_link_libraries(
    mqttserver PUBLIC snodec::mqtt snodec::net-in-stream-legacy
)

set_target_properties(mqttserver PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

install(TARGETS mqttserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(
    topictreetest
    topictreetest.cpp
    server/TopicTree.cpp
    server/SubscriberTree.cpp
    server/TopicTree.h
    server/SubscriberTree.h
    server/SocketContext.cpp
    server/SocketContext.h
    server/Broker.cpp
    server/Broker.h
)

target_include_directories(topictreetest PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(topictreetest PUBLIC snodec::mqtt snodec::logger)

set_target_properties(topictreetest PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

install(TARGETS topictreetest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
