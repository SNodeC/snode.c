cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(MQTT1_PACKETS_CPP
    Connect.cpp
    Connack.cpp
    Disconnect.cpp
    Pingreq.cpp
    Pingresp.cpp
    Publish.cpp
    Puback.cpp
    Pubcomp.cpp
    Pubrec.cpp
    Pubrel.cpp
    Subscribe.cpp
    Suback.cpp
    Unsubscribe.cpp
    Unsuback.cpp
)
set(MQTT1_PACKETS_H
    Connect.h
    Connack.h
    Disconnect.h
    Pingreq.h
    Pingresp.h
    Publish.h
    Puback.h
    Pubcomp.h
    Pubrec.h
    Pubrel.h
    Subscribe.h
    Suback.h
    Unsubscribe.h
    Unsuback.h
)

add_library(mqtt1-packets STATIC ${MQTT1_PACKETS_CPP} ${MQTT1_PACKETS_H})
add_library(snodec::mqtt1-packets ALIAS mqtt1-packets)

target_link_libraries(mqtt1-packets PRIVATE snodec::mqtt1-types)

set_property(TARGET mqtt1-packets PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt1-packets
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/iot/mqtt1/packets
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
