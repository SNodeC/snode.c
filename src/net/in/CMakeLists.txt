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

set(NET-IN_CPP PhysicalSocket.cpp SocketAddress.cpp SocketAddrInfo.cpp
               config/ConfigAddress.cpp
)

set(NET-IN_H PhysicalSocket.hpp PhysicalSocket.h SocketAddress.h
             SocketAddrInfo.h config/ConfigAddress.h
)

add_library(net-in SHARED ${NET-IN_CPP} ${NET-IN_H})
add_library(snodec::net-in ALIAS net-in)

target_link_libraries(net-in PUBLIC snodec::net snodec::core-socket)

target_include_directories(
    net-in
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in PROPERTIES SOVERSION ${SNODEC_SO_VERSION} OUTPUT_NAME snodec-net-in
)

install(
    TARGETS net-in
    EXPORT snodec_net-in_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-in
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in
    COMPONENT net-in
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "stream" EXCLUDE
    PATTERN "dgram" EXCLUDE
)

install(
    EXPORT snodec_net-in_Targets
    FILE snodec_net-in_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-in
)

add_subdirectory(stream)
