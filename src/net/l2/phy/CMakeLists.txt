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

set(NET-L2-PHY_CPP PhysicalSocket.cpp)

set(NET-L2-PHY_H PhysicalSocket.hpp PhysicalSocket.h)

add_library(net-l2-phy SHARED ${NET-L2-PHY_CPP} ${NET-L2-PHY_H})
add_library(snodec::net-l2-phy ALIAS net-l2-phy)

target_link_libraries(net-l2-phy PUBLIC net-l2 ${BLUEZ_LIBRARIES})

target_include_directories(
    net-l2-phy
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-l2-phy PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                                        snodec-net-l2-phy
)

install(
    TARGETS net-l2-phy
    EXPORT snodec_net-l2-phy_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-l2-phy
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/l2/phy
    COMPONENT net-l2-phy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "stream" EXCLUDE
    PATTERN "dgram" EXCLUDE
)

install(
    EXPORT snodec_net-l2-phy_Targets
    FILE snodec_net-l2-phy_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-l2-phy
)

add_subdirectory(stream)
