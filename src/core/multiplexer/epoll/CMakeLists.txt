# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025
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
#
# ---------------------------------------------------------------------------
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 3.18)

set(MUX_EPOLL_CPP DescriptorEventPublisher.cpp EventMultiplexer.cpp)

set(MUX_EPOLL_H DescriptorEventPublisher.h EventMultiplexer.h)

add_library(mux-epoll SHARED ${MUX_EPOLL_CPP} ${MUX_EPOLL_H})
add_library(snodec::mux-epoll ALIAS mux-epoll)

target_include_directories(
    mux-epoll PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                     "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    mux-epoll
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-core-mux-epoll
)

install(
    TARGETS mux-epoll
    EXPORT snodec_mux-epoll_Targets
    LIBRARY DESTINATION ${CMAKE_INISTALL_LIBDIR} COMPONENT mux-epoll
)

install(
    EXPORT snodec_mux-epoll_Targets
    FILE snodec_mux-epoll_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mux-epoll
)
