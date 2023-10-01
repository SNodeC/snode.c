# snode.c - a slim toolkit for network communication
# Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.18)

find_package(PkgConfig REQUIRED)

pkg_check_modules(NLOHMANN_JSON nlohmann_json>=3.11)

if(NLOHMANN_JSON_FOUND)
    set(MQTT_INSTALL_DIR "snode.c/iot/mqtt")
    set(MQTT_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/${MQTT_INSTALL_DIR})
    set(MQTT_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/${MQTT_INSTALL_DIR})
    set(MQTT_RPATH ${CMAKE_INSTALL_PREFIX}/${MQTT_INSTALL_LIBDIR})

    add_subdirectory(packets)
    add_subdirectory(types)

    set(MQTT_CPP
        ControlPacket.cpp
        ControlPacketDeserializer.cpp
        FixedHeader.cpp
        Mqtt.cpp
        MqttContext.cpp
        Session.cpp
        SocketContext.cpp
        Topic.cpp
    )

    set(MQTT_H
        ControlPacket.h
        ControlPacketDeserializer.h
        FixedHeader.h
        Mqtt.h
        MqttContext.h
        Session.h
        SubProtocol.h
        SubProtocol.hpp
        SocketContext.h
        Topic.h
    )

    add_library(mqtt SHARED ${MQTT_CPP} ${MQTT_H})
    add_library(snodec::mqtt ALIAS mqtt)

    target_include_directories(
        mqtt PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                    "$<INSTALL_INTERFACE:include/snode.c>"
    )

    target_link_libraries(
        mqtt
        PRIVATE snodec::mqtt-packets
        PUBLIC snodec::core-socket-stream ${NLOHMANN_JSON_LIBRARIES}
    )

    set_target_properties(
        mqtt PROPERTIES SOVERSION ${SNODEC_SO_VERSION} OUTPUT_NAME snodec-mqtt
    )

    install(
        TARGETS mqtt
        EXPORT snodec_mqtt_Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${MQTT_INSTALL_DIR}
                COMPONENT mqtt
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${MQTT_INSTALL_INCLUDEDIR}
        COMPONENT mqtt
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "cmake" EXCLUDE
        PATTERN "server" EXCLUDE
        PATTERN "client" EXCLUDE
        PATTERN "types" EXCLUDE
        PATTERN "packets" EXCLUDE
    )

    install(
        EXPORT snodec_mqtt_Targets
        FILE snodec_mqtt_Targets.cmake
        NAMESPACE snodec::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
        COMPONENT mqtt
    )

    add_subdirectory(server)
    add_subdirectory(client)
else()
    message(
        WARNING
            " nlohmann-json3-dev not found:\n"
            "   nlohmann_json is used for MQTT. Plaase install it by executing\n"
            "       sudo apt install nlohmann-json3-dev"
    )
endif()
