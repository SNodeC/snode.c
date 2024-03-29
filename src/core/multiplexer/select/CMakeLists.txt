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

set(MUX_SELECT_CPP DescriptorEventPublisher.cpp EventMultiplexer.cpp)

set(MUX_SELECT_H DescriptorEventPublisher.h EventMultiplexer.h)

add_library(mux-select SHARED ${MUX_SELECT_CPP} ${MUX_SELECT_H})
add_library(snodec::mux-select ALIAS mux-select)

target_include_directories(
    mux-select PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                      "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mux-select PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                                        snodec-core-mux-select
)

install(
    TARGETS mux-select
    EXPORT snodec_mux-select_Targets
    LIBRARY DESTINATION ${CMAKE_INISTALL_LIBDIR} COMPONENT mux-select
)

install(
    EXPORT snodec_mux-select_Targets
    FILE snodec_mux-select_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mux-select
)
