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

get_target_property(HTTP_INSTALL_RPATH http HTTP_INSTALL_RPATH)

add_subdirectory(packets)

set(MQTT_CLIENT_CPP ControlPacketDeserializer.cpp Mqtt.cpp)

set(MQTT_CLIENT_H ControlPacketDeserializer.h Mqtt.h)

add_library(mqtt-client SHARED ${MQTT_CLIENT_CPP} ${MQTT_CLIENT_H})
add_library(snodec::mqtt-client ALIAS mqtt-client)

target_link_libraries(
    mqtt-client
    PUBLIC mqtt
    PRIVATE mqtt-client-packets ${NLOHMANN_JSON_LIBRARIES}
)

target_include_directories(
    mqtt-client PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                       "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mqtt-client
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-mqtt-client
               INSTALL_RPATH "${MQTT_RPATH};${HTTP_INSTALL_RPATH}/upgrade"
)

install(
    TARGETS mqtt-client
    EXPORT snodec_mqtt-client_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${MQTT_INSTALL_DIR}
            COMPONENT mqtt-client
)

install(
    FILES Mqtt.h
    DESTINATION ${MQTT_INSTALL_INCLUDEDIR}/client
    COMPONENT mqtt-client
)

install(
    EXPORT snodec_mqtt-client_Targets
    FILE snodec_mqtt-client_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mqtt-client
)

set(MQTT_CLIENT_WEBSOCKET_CPP SubProtocol.cpp)

set(MQTT_CLIENT_WEBSOCKET_H SubProtocol.h)

add_library(
    mqtt-client-websocket SHARED ${MQTT_CLIENT_WEBSOCKET_CPP}
                                 ${MQTT_CLIENT_WEBSOCKET_H}
)
add_library(snodec::mqtt-client-websocket ALIAS mqtt-client-websocket)

target_link_libraries(
    mqtt-client-websocket
    PUBLIC mqtt-client websocket-client
    PRIVATE mqtt-broker mqtt-client-packets
)

target_include_directories(
    mqtt-client-websocket PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                                 "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mqtt-client-websocket
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-mqtt-client-websocket
               INSTALL_RPATH "${MQTT_RPATH};${HTTP_INSTALL_RPATH}/upgrade"
)

install(
    TARGETS mqtt-client-websocket
    EXPORT snodec_mqtt-client-websocket_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${MQTT_INSTALL_DIR}
            COMPONENT mqtt-client-websocket
)

install(
    FILES SubProtocol.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${MQTT_INSTALL_DIR}/client
    COMPONENT mqtt-client-websocket
)

install(
    EXPORT snodec_mqtt-client-websocket_Targets
    FILE snodec_mqtt-client-websocket_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mqtt-client-websocket
)
