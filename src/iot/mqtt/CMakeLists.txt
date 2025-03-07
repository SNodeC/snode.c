# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025
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
#
# ---------------------------------------------------------------------------
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

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
        SubProtocol.cpp
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
        PRIVATE mqtt-packets
        PUBLIC core-socket-stream ${NLOHMANN_JSON_LIBRARIES}
    )

    set_target_properties(
        mqtt
        PROPERTIES VERSION ${SNode.C_VERSION}
                   SOVERSION ${SNODEC_SOVERSION}
                   OUTPUT_NAME snodec-mqtt
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
