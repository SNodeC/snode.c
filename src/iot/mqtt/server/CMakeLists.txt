cmake_minimum_required(VERSION 3.18)

add_subdirectory(packets)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(MQTT_SERVER_CPP SocketContext.cpp)

set(MQTT_SERVER_H SocketContext.h)

add_library(mqtt-server SHARED ${MQTT_SERVER_CPP} ${MQTT_SERVER_H})
add_library(snodec::mqtt-server ALIAS mqtt-server)

target_link_libraries(
    mqtt-server
    PUBLIC snodec::mqtt
    PRIVATE snodec::mqtt-server-packets
)

target_include_directories(
    mqtt-server PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                       "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mqtt-server PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-mqtt-server
)

install(
    TARGETS mqtt-server
    EXPORT snodec_mqtt-server_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/iot/mqtt
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/iot/mqtt/server
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "types" EXCLUDE
    PATTERN "packets" EXCLUDE
)

install(
    EXPORT snodec_mqtt-server_Targets
    FILE snodec_mqtt-server_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
