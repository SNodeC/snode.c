cmake_minimum_required(VERSION 3.18)

add_subdirectory(packets)
add_subdirectory(types)

set(SNODEC_IOT_MQTT_RPATH
    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/snode.c/iot/mqtt"
    CACHE INTERNAL ""
)

set(MQTT_CPP
    ControlPacket.cpp
    ControlPacketDeserializer.cpp
    FixedHeader.cpp
    Mqtt.cpp
    MqttContext.cpp
    Session.cpp
    SocketContext.cpp
    Topic.cpp
)

set(MQTT_H
    ControlPacket.h
    ControlPacketDeserializer.h
    FixedHeader.h
    Mqtt.h
    MqttContext.h
    Session.h
    SubProtocol.h
    SubProtocol.hpp
    SocketContext.h
    Topic.h
)

add_library(mqtt SHARED ${MQTT_CPP} ${MQTT_H})
add_library(snodec::mqtt ALIAS mqtt)

target_include_directories(
    mqtt PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mqtt
    PROPERTIES SOVERSION 1
               OUTPUT_NAME snodec-mqtt
               IOT_MQTT_RPATH "${SNODEC_IOT_MQTT_RPATH}"
               EXPORT_PROPERTIES "IOT_MQTT_RPATH"
)

install(
    TARGETS mqtt
    EXPORT snodec_mqtt_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/iot/mqtt COMPONENT mqtt
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/iot/mqtt
    COMPONENT mqtt
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "server" EXCLUDE
    PATTERN "client" EXCLUDE
    PATTERN "types" EXCLUDE
    PATTERN "packets" EXCLUDE
)

install(
    EXPORT snodec_mqtt_Targets
    FILE snodec_mqtt_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mqtt
)

add_subdirectory(server)
add_subdirectory(client)
