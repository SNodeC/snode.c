cmake_minimum_required(VERSION 3.0)

add_subdirectory(types)
add_subdirectory(packets)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(MQTT_CPP
    ControlPacket.cpp
    ControlPacketFactory.cpp
    SocketContext.cpp
    Receiver.cpp
    UTF8StringEncoder.cpp
    UTF8StringDecoder.cpp
    VarInteger.cpp
)
set(MQTT_H
    ControlPacket.h
    ControlPacketFactory.h
    SocketContext.h
    Receiver.h
    UTF8StringEncoder.h
    UTF8StringDecoder.h
    VarInteger.h
)

add_library(mqtt SHARED ${MQTT_CPP} ${MQTT_H})
add_library(snodec::mqtt ALIAS mqtt)

target_include_directories(
    mqtt
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${LIBMAGIC_INCLUDE_DIRS}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(mqtt PUBLIC snodec::core-socket PRIVATE snodec::mqtt-types snodec::mqtt-packets)

install(
    TARGETS mqtt
    EXPORT snodec_mqtt_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/mqtt
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/mqtt
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_mqtt_Targets
    FILE mqtt_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
