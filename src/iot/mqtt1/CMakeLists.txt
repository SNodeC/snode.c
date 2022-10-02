cmake_minimum_required(VERSION 3.18)

set(CMAKE_INSTALL_RPATH
    ${CMAKE_INSTALL_RPATH} "${CMAKE_INSTALL_PREFIX}/lib/snode.c/iot/mqtt1"
    CACHE INTERNAL ""
)

add_subdirectory(packets)
add_subdirectory(types)


set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(MQTT_CPP ControlPacket.cpp SocketContext.cpp Topic.cpp
)
set(MQTT_H ControlPacket.h SocketContext.h Topic.h)

add_library(mqtt1 SHARED ${MQTT_CPP} ${MQTT_H})
add_library(snodec::mqtt1 ALIAS mqtt1)

target_link_libraries(mqtt1 PRIVATE snodec::mqtt1-types snodec::mqtt1-packets)

target_include_directories(
    mqtt1
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${LIBMAGIC_INCLUDE_DIRS}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(mqtt1 PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-mqtt)

install(
    TARGETS mqtt1
    EXPORT snodec_mqtt_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/iot/mqtt
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/iot/mqtt
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_mqtt_Targets
    FILE snodec_mqtt_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
