cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(MQTT_PACKETS_SERIALIZER_CPP
    Connect.cpp
    Connack.cpp
    Disconnect.cpp
    Pingreq.cpp
    Pingresp.cpp
    Subscribe.cpp
    Suback.cpp
    Unsubscribe.cpp
    Unsuback.cpp
)
set(MQTT_PACKETS_SERIALIZER_H
    Connect.h
    Connack.h
    Disconnect.h
    Pingreq.h
    Pingresp.h
    Subscribe.h
    Suback.h
    Unsubscribe.h
    Unsuback.h
)

add_library(mqtt-packets-serializer STATIC ${MQTT_PACKETS_SERIALIZER_CPP} ${MQTT_PACKETS_SERIALIZER_H})
add_library(snodec::mqtt-packets-serializer ALIAS mqtt-packets-serializer)

target_link_libraries(mqtt-packets-serializer PRIVATE snodec::mqtt-packets)

set_property(TARGET mqtt-packets-serializer PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt-packets-serializer PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/iot/mqtt/packets/serializer
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
