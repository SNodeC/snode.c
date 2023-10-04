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

set(NET-RC-PHY-STREAM_CPP
    PhysicalSocket.cpp PhysicalSocketClient.cpp PhysicalSocketServer.cpp
)

set(NET-RC-PHY-STREAM_H
    PhysicalSocketClient.h
    PhysicalSocketServer.h
    PhysicalSocket.hpp
    PhysicalSocket.h
)

add_library(net-rc-phy-stream SHARED ${NET-RC-PHY-STREAM_CPP} ${NET-RC-PHY-STREAM_H})
add_library(snodec::net-rc-phy-stream ALIAS net-rc-phy-stream)

target_link_libraries(net-rc-phy-stream PUBLIC snodec::net-rc-phy)

target_include_directories(
    net-rc-phy-stream
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-rc-phy-stream PROPERTIES SOVERSION ${SNODEC_SOVERSION}
                              OUTPUT_NAME snodec-net-rc-phy-stream
)

install(
    TARGETS net-rc-phy-stream
    EXPORT snodec_net-rc-phy-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-rc-stream
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/rc/phy/stream
    COMPONENT net-rc-phy-stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "legacy" EXCLUDE
    PATTERN "tls" EXCLUDE
)

install(
    EXPORT snodec_net-rc-phy-stream_Targets
    FILE snodec_net-rc-phy-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-rc-phy-stream
)
