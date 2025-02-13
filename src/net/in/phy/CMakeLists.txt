# SNode.C - a slim toolkit for network communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025
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

set(NET-IN-PHY_CPP PhysicalSocket.cpp)

set(NET-IN-PHY_H PhysicalSocket.hpp PhysicalSocket.h)

add_library(net-in-phy SHARED ${NET-IN-PHY_CPP} ${NET-IN-PHY_H})
add_library(snodec::net-in-phy ALIAS net-in-phy)

target_link_libraries(net-in-phy PUBLIC net-in)

target_include_directories(
    net-in-phy PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                      "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in-phy PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                                        snodec-net-in-phy
)

install(
    TARGETS net-in-phy
    EXPORT snodec_net-in-phy_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-in-phy
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in/phy
    COMPONENT net-in-phy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "stream" EXCLUDE
    PATTERN "dgram" EXCLUDE
)

install(
    EXPORT snodec_net-in-phy_Targets
    FILE snodec_net-in-phy_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-in-phy
)

add_subdirectory(stream)
