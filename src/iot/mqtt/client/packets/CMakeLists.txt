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
