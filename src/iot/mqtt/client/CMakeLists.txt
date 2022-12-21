cmake_minimum_required(VERSION 3.18)

add_subdirectory(packets)

set(MQTT_CLIENT_CPP Mqtt.cpp)

set(MQTT_CLIENT_H ControlPacketDeserializer.h Mqtt.h)

add_library(mqtt-client SHARED ${MQTT_CLIENT_CPP} ${MQTT_CLIENT_H})
add_library(snodec::mqtt-client ALIAS mqtt-client)

target_link_libraries(
    mqtt-client
    PUBLIC snodec::mqtt
    PRIVATE snodec::mqtt-client-packets
)

target_include_directories(
    mqtt-client PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                       "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mqtt-client
    PROPERTIES SOVERSION 1
               OUTPUT_NAME snodec-mqtt-client
               IOT_MQTT_CLIENT_INCLUDE_DIRS
               "${CMAKE_INSTALL_PREFIX}/include/snode.c"
               INSTALL_RPATH "${SNODEC_IOT_MQTT_RPATH}"
)

install(
    TARGETS mqtt-client
    EXPORT snodec_mqtt-client_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/iot/mqtt
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/iot/mqtt/client
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "packets" EXCLUDE
)

install(
    EXPORT snodec_mqtt-client_Targets
    FILE snodec_mqtt-client_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
