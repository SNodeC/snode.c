cmake_minimum_required(VERSION 3.18)

set(MQTT_PACKETS_CLIENT_CPP
    Connack.cpp
    Pingresp.cpp
    Publish.cpp
    Puback.cpp
    Pubcomp.cpp
    Pubrec.cpp
    Pubrel.cpp
    Suback.cpp
    Unsuback.cpp
)
set(MQTT_PACKETS_CLIENT_H
    Connack.h
    Pingresp.h
    Publish.h
    Puback.h
    Pubcomp.h
    Pubrec.h
    Pubrel.h
    Suback.h
    Unsuback.h
)

add_library(
    mqtt-client-packets STATIC ${MQTT_PACKETS_CLIENT_CPP}
                               ${MQTT_PACKETS_CLIENT_H}
)
add_library(snodec::mqtt-client-packets ALIAS mqtt-client-packets)

target_link_libraries(mqtt-client-packets PRIVATE snodec::mqtt-packets)

set_property(TARGET mqtt-client-packets PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt-client-packets PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${MQTT_INSTALL_INCLUDEDIR}/client/packets
    COMPONENT mqtt-client
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
