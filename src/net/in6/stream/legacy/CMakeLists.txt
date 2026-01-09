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

set(NET-IN6-STREAM-LEGACY_CPP
    SocketClient.cpp SocketServer.cpp config/ConfigSocketClient.cpp
    config/ConfigSocketServer.cpp
)

set(NET-IN6-STREAM-LEGACY_H
    SocketClient.h SocketServer.h config/ConfigSocketClient.h
    config/ConfigSocketServer.h
)

add_library(
    net-in6-stream-legacy SHARED ${NET-IN6-STREAM-LEGACY_CPP}
                                 ${NET-IN6-STREAM-LEGACY_H}
)
add_library(snodec::net-in6-stream-legacy ALIAS net-in6-stream-legacy)

target_link_libraries(
    net-in6-stream-legacy PUBLIC net-in6-stream core-socket-stream-legacy
)

target_include_directories(
    net-in6-stream-legacy PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                                 "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in6-stream-legacy
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-net-in6-stream-legacy
)

install(
    TARGETS net-in6-stream-legacy
    EXPORT snodec_net-in6-stream-legacy_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-in6-stream-legacy
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in6/stream/legacy
    COMPONENT net-in6-stream-legacy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-in6-stream-legacy_Targets
    FILE snodec_net-in6-stream-legacy_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-in6-stream-legacy
)
