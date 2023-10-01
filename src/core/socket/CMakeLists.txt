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

set(CORE_SOCKET_CPP SocketAddress.cpp SocketContext.cpp)

set(CORE_SOCKET_H LogicalSocket.h LogicalSocket.hpp SocketAddress.h
                  SocketContext.h
)

add_library(core-socket SHARED ${CORE_SOCKET_CPP} ${CORE_SOCKET_H})
add_library(snodec::core-socket ALIAS core-socket)

target_include_directories(
    core-socket
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(core-socket PUBLIC snodec::core)

set_target_properties(
    core-socket PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                                         snodec-core-socket
)

install(
    TARGETS core-socket
    EXPORT snodec_core-socket_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT core-socket
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/socket
    COMPONENT core-socket
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "." EXCLUDE
    PATTERN "stream" EXCLUDE
    PATTERN "dgram" EXCLUDE
)

install(
    EXPORT snodec_core-socket_Targets
    FILE snodec_core-socket_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT core-socket
)

add_subdirectory(stream)
