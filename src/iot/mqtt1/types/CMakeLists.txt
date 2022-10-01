cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(MQTT_TYPES_CPP String.cpp StringRaw.cpp UInt16.cpp UInt32.cpp UInt64.cpp UInt8.cpp UInt8List.cpp UIntV.cpp)
set(MQTT_TYPES_H String.h StringRaw.h UInt16.h UInt32.h UInt64.h UInt8.h UInt8List.h UIntV.h)

add_library(mqtt1-types STATIC ${MQTT_TYPES_CPP} ${MQTT_TYPES_H})
add_library(snodec::mqtt1-types ALIAS mqtt-types)

set_property(TARGET mqtt1-types PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt1-types
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${LIBMAGIC_INCLUDE_DIRS}>"
)
