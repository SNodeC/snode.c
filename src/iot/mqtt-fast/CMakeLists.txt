cmake_minimum_required(VERSION 3.18)

add_subdirectory(types)
add_subdirectory(packets)

set(SNODEC_IOT_MQTT-FAST_RPATH
    ""
    # "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/snode.c/iot/mqtt"
    CACHE INTERNAL ""
)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(MQTT_FAST_CPP ControlPacket.cpp ControlPacketFactory.cpp SocketContext.cpp
                  Topic.cpp
)
set(MQTT_FAST_H ControlPacket.h ControlPacketFactory.h SocketContext.h Topic.h)

add_library(mqtt-fast SHARED ${MQTT_FAST_CPP} ${MQTT_FAST_H})
add_library(snodec::mqtt-fast ALIAS mqtt-fast)

target_link_libraries(
    mqtt-fast PRIVATE snodec::mqtt-fast-types snodec::mqtt-fast-packets
                      snodec::core-socket-stream
)

target_include_directories(
    mqtt-fast PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                     "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mqtt-fast PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                                       snodec-mqtt-fast
)

install(
    TARGETS mqtt-fast
    EXPORT snodec_mqtt-fast_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/iot/mqtt
            COMPONENT mqtt-fast
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/iot/mqtt-fast
    COMPONENT mqtt-fast
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "types" EXCLUDE
    PATTERN "packets" EXCLUDE
)

install(
    EXPORT snodec_mqtt-fast_Targets
    FILE snodec_mqtt-fast_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mqtt-fast
)
