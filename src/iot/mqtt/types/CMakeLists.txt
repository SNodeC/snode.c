cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(mqtt_TYPES_CPP
    String.cpp
    StringRaw.cpp
    UInt16.cpp
    UInt32.cpp
    UInt64.cpp
    UInt8.cpp
    UInt8List.cpp
    UIntV.cpp
)
set(mqtt_TYPES_H
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

add_library(mqtt-types STATIC ${mqtt_TYPES_CPP} ${mqtt_TYPES_H})
add_library(snodec::mqtt-types ALIAS mqtt-types)

set_property(TARGET mqtt-types PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt-types
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/iot/mqtt/types
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
