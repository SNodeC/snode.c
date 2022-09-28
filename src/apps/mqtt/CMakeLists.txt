cmake_minimum_required(VERSION 3.5)

project(MQTTBroker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(config.h.in config.h @ONLY)


if(NOT SNODEC_INTREE_BUILD)
    find_package(snodec COMPONENTS net-in-stream-legacy net-in-stream-tls net-un-stream-legacy mqtt)
endif(NOT SNODEC_INTREE_BUILD)

set(MQTTBROKER_CPP
    mqttbroker.cpp
    broker/Broker.cpp
    broker/SharedSocketContextFactory.cpp
    broker/SocketContext.cpp
    broker/SocketContextFactory.cpp
    broker/SubscribtionTree.cpp
    broker/RetainTree.cpp
)
set(MQTTBROKER_H
    broker/Broker.h broker/SharedSocketContextFactory.h broker/SocketContext.h
    broker/SocketContextFactory.h broker/SubscribtionTree.h broker/RetainTree.h
)

add_executable(mqttbroker ${MQTTBROKER_CPP} ${MQTTBROKER_H})

target_include_directories(mqttbroker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(
    mqttbroker PUBLIC snodec::mqtt snodec::net-in-stream-legacy
                      snodec::net-in-stream-tls snodec::net-un-stream-legacy
)

set_target_properties(mqttbroker PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

install(TARGETS mqttbroker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
