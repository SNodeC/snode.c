cmake_minimum_required(VERSION 3.18)

add_subdirectory(packets)

set(MQTT_CLIENT_CPP ControlPacketDeserializer.cpp Mqtt.cpp)

set(MQTT_CLIENT_H ControlPacketDeserializer.h Mqtt.h)

add_library(mqtt-client SHARED ${MQTT_CLIENT_CPP} ${MQTT_CLIENT_H})
add_library(snodec::mqtt-client ALIAS mqtt-client)

target_link_libraries(
    mqtt-client
    PUBLIC snodec::mqtt
    PRIVATE snodec::mqtt-client-packets ${NLOHMANN_JSON_LIBRARIES}
)

target_include_directories(
    mqtt-client PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                       "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mqtt-client
    PROPERTIES SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-mqtt-client
               INSTALL_RPATH ${MQTT_RPATH}
)

install(
    TARGETS mqtt-client
    EXPORT snodec_mqtt-client_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${MQTT_INSTALL_DIR}
            COMPONENT mqtt-client
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${MQTT_INSTALL_INCLUDEDIR}/client
    COMPONENT mqtt-client
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
    COMPONENT mqtt-client
)
