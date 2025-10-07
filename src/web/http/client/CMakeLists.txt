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

add_subdirectory(commands)

set(HTTPCLIENT_CPP
    ConfigHTTP.cpp
    Request.cpp
    RequestCommand.cpp
    Response.cpp
    ResponseParser.cpp
    SocketContext.cpp
    SocketContextFactory.cpp
    SocketContextUpgradeFactory.cpp
    SocketContextUpgradeFactorySelector.cpp
)

set(HTTPCLIENT_H
    Client.h
    ConfigHTTP.h
    Request.h
    RequestCommand.h
    Response.h
    ResponseParser.h
    SocketContext.h
    SocketContextFactory.h
    SocketContextUpgradeFactory.h
    SocketContextUpgradeFactorySelector.h
)

append_source_file_config(
    ConfigHTTP.cpp HTTP_REQUEST_PIPELINED "HTTP pipelined requests" true
)

add_library(http-client SHARED ${HTTPCLIENT_CPP} ${HTTPCLIENT_H}
)
add_library(snodec::http-client ALIAS http-client)

set_source_files_properties(
    SocketContextUpgradeFactorySelector.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        "HTTP_UPGRADE_INSTALL_LIBDIR=\"${CMAKE_INSTALL_PREFIX}/${HTTP_UPGRADE_INSTALL_LIBDIR}\"; \
            SOVERSION=\"${SNODEC_SOVERSION}\""
)

target_include_directories(
    http-client PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                       "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    http-client
    PRIVATE http-client-commands
    PUBLIC http
)

set_target_properties(
    http-client
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-http-client
               LIBRARY_OUTPUT_DIRECTORY "${HTTP_COMPILE_LIBDIR}"
               BUILD_RPATH "${HTTP_COMPILE_LIBDIR}"
               INSTALL_RPATH ${HTTP_INSTALL_RPATH}
               HTTP_INSTALL_RPATH "${HTTP_INSTALL_RPATH}"
               HTTP_COMPILE_LIBDIR "${HTTP_COMPILE_LIBDIR}"
               HTTP_INSTALL_LIBDIR "${HTTP_INSTALL_LIBDIR}"
               HTTP_UPGRADE_COMPILE_LIBDIR "${HTTP_UPGRADE_COMPILE_LIBDIR}"
               HTTP_UPGRADE_INSTALL_LIBDIR "${HTTP_UPGRADE_INSTALL_LIBDIR}"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${HTTP_INSTALL_INCLUDEDIR}/client
    COMPONENT http-client
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../legacy/
    DESTINATION ${HTTP_INSTALL_INCLUDEDIR}/legacy
    COMPONENT http-client
    FILES_MATCHING
    PATTERN "Client.h"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../tls/
    DESTINATION ${HTTP_INSTALL_INCLUDEDIR}/tls
    COMPONENT http-client
    FILES_MATCHING
    PATTERN "Client.h"
)

install(
    TARGETS http-client
    EXPORT snodec_http-client_Targets
    LIBRARY DESTINATION ${HTTP_INSTALL_LIBDIR} COMPONENT http-client
)

install(
    EXPORT snodec_http-client_Targets
    FILE snodec_http-client_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT http-client
)
