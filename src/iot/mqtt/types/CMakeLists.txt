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

set(MQTT_TYPES_CPP
    String.cpp
    StringRaw.cpp
    UInt16.cpp
    UInt32.cpp
    UInt64.cpp
    UInt8.cpp
    UInt8List.cpp
    UIntV.cpp
)
set(MQTT_TYPES_H
    String.h
    StringRaw.h
    TypeBase.h
    TypeBase.hpp
    UInt16.h
    UInt32.h
    UInt64.h
    UInt8.h
    UInt8List.h
    UIntV.h
)

add_library(mqtt-types STATIC ${MQTT_TYPES_CPP} ${MQTT_TYPES_H})
add_library(snodec::mqtt-types ALIAS mqtt-types)

set_target_properties(mqtt-types PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt-types PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${MQTT_INSTALL_INCLUDEDIR}/types
    COMPONENT mqtt
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
