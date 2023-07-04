cmake_minimum_required(VERSION 3.18)

set(SNODEC_IOT_MQTT_SERVER_RPATH "${SNODEC_IOT_MQTT_RPATH};${SNODEC_IOT_MQTT_RPATH}/server")

add_subdirectory(packets)
add_subdirectory(broker)

set(MQTT_SERVER_CPP Mqtt.cpp ControlPacketDeserializer.cpp
                    SharedSocketContextFactory.cpp SocketContextFactory.cpp
)

set(MQTT_SERVER_H ControlPacketDeserializer.h Mqtt.h
                  SharedSocketContextFactory.h SocketContextFactory.h
)

add_library(mqtt-server SHARED ${MQTT_SERVER_CPP} ${MQTT_SERVER_H})
add_library(snodec::mqtt-server ALIAS mqtt-server)

target_link_libraries(
    mqtt-server
    PUBLIC snodec::mqtt
    PRIVATE snodec::mqtt-broker
    PRIVATE snodec::mqtt-server-packets
)

target_include_directories(
    mqtt-server PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                       "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mqtt-server
    PROPERTIES SOVERSION 1
               OUTPUT_NAME snodec-mqtt-server
               INSTALL_RPATH "${SNODEC_IOT_MQTT_RPATH}"
               IOT_MQTT_SERVER_INCLUDE_DIRS
               "${CMAKE_INSTALL_PREFIX}/include/snode.c"
               IOT_MQTT_SERVER_RPATH "${SNODEC_IOT_MQTT_SERVER_RPATH}"
               EXPORT_PROPERTIES "IOT_MQTT_SERVER_RPATH"
)

install(
    TARGETS mqtt-server
    EXPORT snodec_mqtt-server_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/iot/mqtt
            COMPONENT mqtt-server
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/iot/mqtt/server
    COMPONENT mqtt-server
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "broker" EXCLUDE
    PATTERN "packets" EXCLUDE
)

install(
    EXPORT snodec_mqtt-server_Targets
    FILE snodec_mqtt-server_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mqtt-server
)
