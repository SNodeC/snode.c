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

set(CORE-SOCKET-STREAM-LEGACY_CPP SocketReader.cpp SocketWriter.cpp)

set(CORE-SOCKET-STREAM-LEGACY_H
    SocketAcceptor.h
    SocketAcceptor.hpp
    SocketConnection.h
    SocketConnection.hpp
    SocketConnector.h
    SocketConnector.hpp
    SocketReader.h
    SocketWriter.h
)

add_library(
    core-socket-stream-legacy SHARED ${CORE-SOCKET-STREAM-LEGACY_CPP}
                                     ${CORE-SOCKET-STREAM-LEGACY_H}
)
add_library(snodec::core-socket-stream-legacy ALIAS core-socket-stream-legacy)

target_include_directories(
    core-socket-stream-legacy
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
              "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(core-socket-stream-legacy PUBLIC core-socket-stream)

set_target_properties(
    core-socket-stream-legacy
    PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                             snodec-core-socket-stream-legacy
)

install(
    TARGETS core-socket-stream-legacy
    EXPORT snodec_core-socket-stream-legacy_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT core-socket-stream-legacy
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/socket/stream/legacy
    COMPONENT core-socket-stream-legacy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_core-socket-stream-legacy_Targets
    FILE snodec_core-socket-stream-legacy_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT core-socket-stream-legacy
)
