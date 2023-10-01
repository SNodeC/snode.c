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

get_target_property(HTTP_INSTALL_RPATH snodec::http-server HTTP_INSTALL_RPATH)

set(WEBSOCKETSERVER_CPP
    GroupsManager.cpp SocketContextUpgrade.cpp SocketContextUpgradeFactory.cpp
    SubProtocol.cpp SubProtocolFactorySelector.cpp
)

set(WEBSOCKETSERVER_H
    GroupsManager.h SocketContextUpgrade.h SocketContextUpgradeFactory.h
    SubProtocol.h SubProtocolFactorySelector.h
)

add_library(websocket-server SHARED ${WEBSOCKETSERVER_CPP} ${WEBSOCKETSERVER_H})
add_library(snodec::websocket-server ALIAS websocket-server)

set_source_files_properties(
    SubProtocolFactorySelector.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        "WEBSOCKET_SUBPROTOCO_INSTALL_LIBDIR=\"${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}\";SOVERSION=\"${SNODEC_SOVERSION}\""
)

set_target_properties(
    websocket-server
    PROPERTIES
        OUTPUT_NAME "snodec-websocket-server"
        LIBRARY_OUTPUT_DIRECTORY "${HTTP_UPGRADE_COMPILE_LIBDIR}"
        SOVERSION ${SNODEC_SOVERSION}
        BUILD_RPATH "${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}"
        INSTALL_RPATH
        "${HTTP_INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/${HTTP_UPGRADE_INSTALL_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
        "${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
        "${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}"
)

target_include_directories(
    websocket-server
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    websocket-server PUBLIC snodec::websocket snodec::http-server
)

install(
    TARGETS websocket-server
    EXPORT snodec_websocket-server_Targets
    LIBRARY DESTINATION ${HTTP_UPGRADE_INSTALL_LIBDIR}
            COMPONENT websocket-server
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${WEBSOCKET_INSTALL_INCLUDEDIR}/server
    COMPONENT websocket-server
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_websocket-server_Targets
    FILE snodec_websocket-server_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT websocket-server
)
