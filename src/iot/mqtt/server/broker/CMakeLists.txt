# snode.c - a slim toolkit for network communication Copyright (C) 2020, 2021,
# 2022, 2023 Volker Christian <me@vchrist.at>
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

set(IOT_MQTT_SERVER_BROKER_CPP Broker.cpp Message.cpp Session.cpp
                               SubscribtionTree.cpp RetainTree.cpp
)
set(IOT_MQTT_SERVER_BROKER_H Broker.h Message.h Session.h SubscribtionTree.h
                             RetainTree.h
)

add_library(
    mqtt-broker STATIC ${IOT_MQTT_SERVER_BROKER_CPP}
                       ${IOT_MQTT_SERVER_BROKER_H}
)
add_library(snodec::mqtt-broker ALIAS mqtt-broker)

set_property(TARGET mqtt-broker PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt-broker PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

target_link_libraries(mqtt-broker PUBLIC ${NLOHMANN_JSON_LIBRARIES})

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${MQTT_INSTALL_INCLUDEDIR}/server/broker
    COMPONENT mqtt-server
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
