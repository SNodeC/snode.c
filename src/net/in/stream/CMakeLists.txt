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

set(NET-IN-STREAM_CPP config/ConfigSocketClient.cpp
                      config/ConfigSocketServer.cpp
)

set(NET-IN-STREAM_H SocketClient.h SocketServer.h config/ConfigSocketClient.h
                    config/ConfigSocketServer.h
)

append_source_file_config(
    config/ConfigSocketServer.cpp REUSE_PORT "Reuse port (IPv4/IPv6)" \"false\"
    \"true\"
)

add_library(net-in-stream SHARED ${NET-IN-STREAM_CPP} ${NET-IN-STREAM_H})
add_library(snodec::net-in-stream ALIAS net-in-stream)

target_link_libraries(net-in-stream PUBLIC net-in-phy-stream)

target_include_directories(
    net-in-stream
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in-stream PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                                           snodec-net-in-stream
)

install(
    TARGETS net-in-stream
    EXPORT snodec_net-in-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-in-stream
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in/stream
    COMPONENT net-in-stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "legacy" EXCLUDE
    PATTERN "tls" EXCLUDE
)

install(
    EXPORT snodec_net-in-stream_Targets
    FILE snodec_net-in-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-in-stream
)

add_subdirectory(legacy)
add_subdirectory(tls)
