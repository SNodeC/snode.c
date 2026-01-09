# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

get_target_property(HTTP_INSTALL_RPATH http-client HTTP_INSTALL_RPATH)

set(WEBSOCKETCLIENT_CPP
    SocketContextUpgrade.cpp SocketContextUpgradeFactory.cpp SubProtocol.cpp
    SubProtocolFactorySelector.cpp
)

set(WEBSOCKETCLIENT_H SocketContextUpgrade.h SocketContextUpgradeFactory.h
                      SubProtocol.h SubProtocolFactorySelector.h
)

include(CheckSymbolExists)
check_symbol_exists(getentropy "sys/random.h" HAVE_GETENTROPY)

if(${HAVE_GETENTROPY})
    set_property(
        SOURCE SocketContextUpgradeFactory.cpp
        APPEND
        PROPERTY COMPILE_DEFINITIONS HAVE_GETENTROPY=YES
    )
    check_symbol_exists(SYS_getrandom "sys/syscall.h" SYS_GETRANDOM)
    if(${SYS_getrandom})
        set_property(
            SOURCE SocketContextUpgradeFactory.cpp
            APPEND
            PROPERTY COMPILE_DEFINITIONS SYS_GETRANDOM=YES
        )
    endif(${SYS_getrandom})
endif(${HAVE_GETENTROPY})

add_library(websocket-client SHARED ${WEBSOCKETCLIENT_CPP} ${WEBSOCKETCLIENT_H})
add_library(snodec::websocket-client ALIAS websocket-client)

set_source_files_properties(
    SubProtocolFactorySelector.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        "WEBSOCKET_SUBPROTOCO_INSTALL_LIBDIR=\"${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}\";SOVERSION=\"${SNODEC_SOVERSION}\""
)

set_target_properties(
    websocket-client
    PROPERTIES
        OUTPUT_NAME "snodec-websocket-client"
        LIBRARY_OUTPUT_DIRECTORY "${HTTP_UPGRADE_COMPILE_LIBDIR}"
        VERSION ${SNode.C_VERSION}
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
    websocket-client PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                            "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(websocket-client PUBLIC websocket http-client)

install(
    TARGETS websocket-client
    EXPORT snodec_websocket-client_Targets
    LIBRARY DESTINATION ${HTTP_UPGRADE_INSTALL_LIBDIR}
            COMPONENT websocket-client
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${WEBSOCKET_INSTALL_INCLUDEDIR}/client
    COMPONENT websocket-client
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_websocket-client_Targets
    FILE snodec_websocket-client_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT websocket-client
)
