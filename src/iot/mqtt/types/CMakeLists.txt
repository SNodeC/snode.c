cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(MQTT_TYPES_CPP Binary.cpp Int_1.cpp Int_V.cpp TypeBase.cpp)
set(MQTT_TYPES_H Binary.h Int_1.h Int_V.h TypeBase.h)

add_library(mqtt-types STATIC ${MQTT_TYPES_CPP} ${MQTT_TYPES_H})
add_library(snodec::mqtt-types ALIAS mqtt-types)

set_property(TARGET mqtt-types PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt-types
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${LIBMAGIC_INCLUDE_DIRS}>"
)
