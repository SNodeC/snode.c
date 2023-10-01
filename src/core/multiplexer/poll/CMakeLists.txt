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

set(MUX_POLL_CPP DescriptorEventPublisher.cpp EventMultiplexer.cpp)

set(MUX_POLL_H DescriptorEventPublisher.h EventMultiplexer.h)

add_library(mux-poll SHARED ${MUX_POLL_CPP} ${MUX_POLL_H})
add_library(snodec::mux-poll ALIAS mux-poll)

target_include_directories(
    mux-poll
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mux-poll PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                                      snodec-core-mux-poll
)

install(
    TARGETS mux-poll
    EXPORT snodec_mux-poll_Targets
    LIBRARY DESTINATION ${CMAKE_INISTALL_LIBDIR} COMPONENT mux-poll
)

install(
    EXPORT snodec_mux-poll_Targets
    FILE snodec_mux-poll_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mux-poll
)
