cmake_minimum_required(VERSION 3.18)

get_target_property(HTTP_INSTALL_RPATH snodec::http HTTP_INSTALL_RPATH)

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
    PRIVATE snodec::mqtt-broker snodec::mqtt-server-packets
)

target_include_directories(
    mqtt-server PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                       "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mqtt-server
    PROPERTIES SOVERSION ${SNODEC_SO_VERSION}
               OUTPUT_NAME snodec-mqtt-server
               INSTALL_RPATH "${MQTT_RPATH};${HTTP_INSTALL_RPATH}/upgrade"
)

install(
    TARGETS mqtt-server
    EXPORT snodec_mqtt-server_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${MQTT_INSTALL_DIR}
            COMPONENT mqtt-server
)

install(
    FILES Mqtt.h SharedSocketContextFactory.h SocketContextFactory.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${MQTT_INSTALL_DIR}/server
    COMPONENT mqtt-server
)

install(
    EXPORT snodec_mqtt-server_Targets
    FILE snodec_mqtt-server_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mqtt-server
)

# ##############################################################################

set(MQTT_SERVER_WEBSOCKET_CPP SubProtocol.cpp)

set(MQTT_SERVER_WEBSOCKET_H SubProtocol.h)

add_library(
    mqtt-server-websocket SHARED ${MQTT_SERVER_WEBSOCKET_CPP}
                                 ${MQTT_SERVER_WEBSOCKET_H}
)
add_library(snodec::mqtt-server-websocket ALIAS mqtt-server)

target_link_libraries(
    mqtt-server-websocket
    PUBLIC snodec::mqtt-server snodec::websocket-server
    PRIVATE snodec::mqtt-broker snodec::mqtt-server-packets
)

target_include_directories(
    mqtt-server-websocket PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                                 "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mqtt-server-websocket
    PROPERTIES SOVERSION ${SNODEC_SO_VERSION}
               OUTPUT_NAME snodec-mqtt-server-websocket
               INSTALL_RPATH "${MQTT_RPATH};${HTTP_INSTALL_RPATH}/upgrade"
)

install(
    TARGETS mqtt-server-websocket
    EXPORT snodec_mqtt-server-websocket_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${MQTT_INSTALL_DIR}
            COMPONENT mqtt-server-websocket
)

install(
    FILES SubProtocol.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${MQTT_INSTALL_DIR}/server
    COMPONENT mqtt-server-websocket
)

install(
    EXPORT snodec_mqtt-server-websocket_Targets
    FILE snodec_mqtt-server-websocket_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mqtt-server-websocket
)
