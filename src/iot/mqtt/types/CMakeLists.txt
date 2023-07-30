cmake_minimum_required(VERSION 3.18)

set(MQTT_TYPES_CPP
    String.cpp
    StringRaw.cpp
    UInt16.cpp
    UInt32.cpp
    UInt64.cpp
    UInt8.cpp
    UInt8List.cpp
    UIntV.cpp
)
set(MQTT_TYPES_H
    String.h
    StringRaw.h
    TypeBase.h
    TypeBase.hpp
    UInt16.h
    UInt32.h
    UInt64.h
    UInt8.h
    UInt8List.h
    UIntV.h
)

add_library(mqtt-types STATIC ${MQTT_TYPES_CPP} ${MQTT_TYPES_H})
add_library(snodec::mqtt-types ALIAS mqtt-types)

set_target_properties(mqtt-types PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt-types PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${MQTT_INSTALL_INCLUDEDIR}/types
    COMPONENT mqtt
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
