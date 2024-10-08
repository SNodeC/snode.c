# SNode.C - a slim toolkit for network communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.18)

set(MQTT_PACKETS_SERVER_CPP
    Connect.cpp
    Disconnect.cpp
    Pingreq.cpp
    Publish.cpp
    Puback.cpp
    Pubcomp.cpp
    Pubrec.cpp
    Pubrel.cpp
    Subscribe.cpp
    Unsubscribe.cpp
)
set(MQTT_PACKETS_SERVER_H
    Connect.h
    Disconnect.h
    Pingreq.h
    Publish.h
    Puback.h
    Pubcomp.h
    Pubrec.h
    Pubrel.h
    Subscribe.h
    Unsubscribe.h
)

add_library(
    mqtt-server-packets STATIC ${MQTT_PACKETS_SERVER_CPP}
                               ${MQTT_PACKETS_SERVER_H}
)
add_library(snodec::mqtt-server-packets ALIAS mqtt-server-packets)

target_link_libraries(mqtt-server-packets PRIVATE mqtt-packets)

set_property(TARGET mqtt-server-packets PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt-server-packets PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${MQTT_INSTALL_INCLUDEDIR}/server/packets
    COMPONENT mqtt-server
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
