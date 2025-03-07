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

find_package(PkgConfig REQUIRED)

pkg_check_modules(BLUEZ bluez)

if(BLUEZ_FOUND)
    set(NET-L2_CPP SocketAddress.cpp config/ConfigAddress.cpp)

    set(NET-L2_H SocketAddress.h config/ConfigAddress.h)

    add_library(net-l2 SHARED ${NET-L2_CPP} ${NET-L2_H})
    add_library(snodec::net-l2 ALIAS net-l2)

    target_link_libraries(net-l2 PUBLIC net)

    target_include_directories(
        net-l2 PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                      "$<INSTALL_INTERFACE:include/snode.c>"
    )

    set_target_properties(
        net-l2
        PROPERTIES VERSION ${SNode.C_VERSION}
                   SOVERSION ${SNODEC_SOVERSION}
                   OUTPUT_NAME snodec-net-l2
    )

    install(
        TARGETS net-l2
        EXPORT snodec_net-l2_Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-l2
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/l2
        COMPONENT net-l2
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "cmake" EXCLUDE
        PATTERN "stream" EXCLUDE
        PATTERN "dgram" EXCLUDE
        PATTERN "phy" EXCLUDE
    )

    install(
        EXPORT snodec_net-l2_Targets
        FILE snodec_net-l2_Targets.cmake
        NAMESPACE snodec::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
        COMPONENT net-l2
    )

    add_subdirectory(phy)
    add_subdirectory(stream)

else(BLUEZ_FOUND)
    message(
        WARNING
            " libbluetooth-dev not found:\n"
            "    If you want to get bluetooth support you should install libbluetooth-dev."
    )
endif(BLUEZ_FOUND)
