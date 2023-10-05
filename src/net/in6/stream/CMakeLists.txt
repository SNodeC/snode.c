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

set(NET-IN6-STREAM_CPP config/ConfigSocketClient.cpp
                       config/ConfigSocketServer.cpp
)

set(NET-IN6-STREAM_H SocketClient.h SocketServer.h config/ConfigSocketClient.h
                     config/ConfigSocketServer.h
)

append_source_file_config(
    config/ConfigSocketServer.cpp REUSE_PORT "Reuse port (IPv4/IPv6)" \"false\"
    \"true\"
)
append_source_file_config(
    config/ConfigSocketServer.cpp IPV6_ONLY "IPv6 only" \"false\" \"true\"
)

add_library(net-in6-stream SHARED ${NET-IN6-STREAM_CPP} ${NET-IN6-STREAM_H})
add_library(snodec::net-in6-stream ALIAS net-in6-stream)

target_link_libraries(
    net-in6-stream PUBLIC snodec::net-in6-phy-stream
)

target_include_directories(
    net-in6-stream
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in6-stream PROPERTIES SOVERSION ${SNODEC_SOVERSION}
                              OUTPUT_NAME snodec-net-in6-stream
)

install(
    TARGETS net-in6-stream
    EXPORT snodec_net-in6-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-in6-stream
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in6/stream
    COMPONENT net-in6-stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "legacy" EXCLUDE
    PATTERN "tls" EXCLUDE
)

install(
    EXPORT snodec_net-in6-stream_Targets
    FILE snodec_net-in6-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-in6-stream
)

add_subdirectory(legacy)
add_subdirectory(tls)
