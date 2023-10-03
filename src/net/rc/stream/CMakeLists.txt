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

set(NET-RC-STREAM_CPP
    PhysicalSocket.cpp PhysicalSocketClient.cpp PhysicalServerSocket.cpp
    config/ConfigSocketClient.cpp config/ConfigSocketServer.cpp
)

set(NET-RC-STREAM_H
    PhysicalSocketClient.h
    PhysicalServerSocket.h
    SocketClient.h
    SocketServer.h
    PhysicalSocket.hpp
    PhysicalSocket.h
    config/ConfigSocketClient.h
    config/ConfigSocketServer.h
)

add_library(net-rc-stream SHARED ${NET-RC-STREAM_CPP} ${NET-RC-STREAM_H})
add_library(snodec::net-rc-stream ALIAS net-rc-stream)

target_link_libraries(net-rc-stream PUBLIC snodec::net-rc)

target_include_directories(
    net-rc-stream
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-rc-stream PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                                           snodec-net-rc-stream
)

install(
    TARGETS net-rc-stream
    EXPORT snodec_net-rc-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-rc-stream
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/rc/stream
    COMPONENT net-rc-stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "legacy" EXCLUDE
    PATTERN "tls" EXCLUDE
)

install(
    EXPORT snodec_net-rc-stream_Targets
    FILE snodec_net-rc-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-rc-stream
)

add_subdirectory(legacy)
add_subdirectory(tls)
