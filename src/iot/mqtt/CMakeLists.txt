cmake_minimum_required(VERSION 3.18)

set(CMAKE_INSTALL_RPATH
    ${CMAKE_INSTALL_RPATH} "${CMAKE_INSTALL_PREFIX}/lib/snode.c/iot/mqtt"
    CACHE INTERNAL ""
)

add_subdirectory(packets)
add_subdirectory(types)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(mqtt_CPP ControlPacket.cpp SocketContext.cpp StaticHeader.cpp Topic.cpp)

set(mqtt_H ControlPacket.h SocketContext.h StaticHeader.h Topic.h)

add_library(mqtt SHARED ${mqtt_CPP} ${mqtt_H})
add_library(snodec::mqtt ALIAS mqtt)

target_link_libraries(mqtt PRIVATE snodec::mqtt-packets)

target_include_directories(
    mqtt
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(mqtt PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-mqtt)

install(
    TARGETS mqtt
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
    PATTERN "types" EXCLUDE
    PATTERN "packets" EXCLUDE
)

install(
    EXPORT snodec_mqtt_Targets
    FILE snodec_mqtt_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
