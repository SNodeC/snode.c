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

get_target_property(
    HTTP_UPGRADE_COMPILE_LIBDIR http HTTP_UPGRADE_COMPILE_LIBDIR
)
get_target_property(
    HTTP_UPGRADE_INSTALL_LIBDIR http HTTP_UPGRADE_INSTALL_LIBDIR
)

set(WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
    "${HTTP_UPGRADE_COMPILE_LIBDIR}/websocket"
)
set(WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
    "${HTTP_UPGRADE_INSTALL_LIBDIR}/websocket"
)
set(WEBSOCKET_INSTALL_INCLUDEDIR
    ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/web/websocket
)

set(WEBSOCKETBASE_CPP Receiver.cpp SubProtocolContext.cpp Transmitter.cpp)
set(WEBSOCKETBASE_H
    Receiver.h
    SocketContextUpgrade.h
    SocketContextUpgrade.hpp
    SubProtocol.h
    SubProtocol.hpp
    SubProtocolContext.h
    SubProtocolFactory.h
    SubProtocolFactorySelector.h
    SubProtocolFactorySelector.hpp
    Transmitter.h
)

add_library(websocket SHARED ${WEBSOCKETBASE_CPP} ${WEBSOCKETBASE_H})
add_library(snodec::websocket ALIAS websocket)

target_include_directories(
    websocket PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                     "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(websocket PUBLIC logger)

set_target_properties(
    websocket
    PROPERTIES OUTPUT_NAME "snodec-websocket"
               LIBRARY_OUTPUT_DIRECTORY "${HTTP_UPGRADE_COMPILE_LIBDIR}"
               VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
               ${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}
               WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
               ${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}
)

install(
    TARGETS websocket
    EXPORT snodec_websocket_Targets
    LIBRARY DESTINATION ${HTTP_UPGRADE_INSTALL_LIBDIR} COMPONENT websocket
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${WEBSOCKET_INSTALL_INCLUDEDIR}
    COMPONENT websocket
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "client" EXCLUDE
    PATTERN "server" EXCLUDE
    PATTERN "subprotocol" EXCLUDE
)

install(
    EXPORT snodec_websocket_Targets
    FILE snodec_websocket_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT websocket
)

add_subdirectory(server)
add_subdirectory(client)

configure_file(
    cmake/snodec_websocket_Targets_Config.cmake.in
    snodec_websocket_Targets_Config.cmake @ONLY
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/snodec_websocket_Targets_Config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT websocket
)
