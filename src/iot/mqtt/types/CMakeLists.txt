cmake_minimum_required(VERSION 3.18)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(DISABLED_WARNINGS -Wno-weak-template-vtables)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

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

set_target_properties(
    mqtt-types PROPERTIES POSITION_INDEPENDENT_CODE ON COMPILE_FLAGS
                                                       "${DISABLED_WARNINGS}"
)

target_include_directories(
    mqtt-types PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/iot/mqtt/types
    COMPONENT mqtt
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
