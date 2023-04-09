cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(MQTT_FAST_TYPES_CPP Binary.cpp Int_1.cpp Int_V.cpp TypeBase.cpp)
set(MQTT_FAST_TYPES_H Binary.h Int_1.h Int_V.h TypeBase.h)

add_library(mqtt-fast-types STATIC ${MQTT_FAST_TYPES_CPP} ${MQTT_FAST_TYPES_H})
add_library(snodec::mqtt-fast-types ALIAS mqtt-fast-types)

set_property(TARGET mqtt-fast-types PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt-fast-types PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/iot/mqtt-fast/types
    COMPONENT mqtt-fast
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
