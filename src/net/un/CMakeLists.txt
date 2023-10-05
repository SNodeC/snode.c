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

set(NET-UN_CPP SocketAddress.cpp config/ConfigAddress.cpp)

set(NET-UN_H SocketAddress.h config/ConfigAddress.h)

add_library(net-un SHARED ${NET-UN_CPP} ${NET-UN_H})
add_library(snodec::net-un ALIAS net-un)

target_link_libraries(net-un PUBLIC snodec::net)

target_include_directories(
    net-un
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-un PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME snodec-net-un
)

install(
    TARGETS net-un
    EXPORT snodec_net-un_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-un
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/un
    COMPONENT net-un
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "stream" EXCLUDE
    PATTERN "dgram" EXCLUDE
    PATTERN "phy" EXCLUDE
)

install(
    EXPORT snodec_net-un_Targets
    FILE snodec_net-un_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-un
)

add_subdirectory(phy)
add_subdirectory(stream)
add_subdirectory(dgram)
