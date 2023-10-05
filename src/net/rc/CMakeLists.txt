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

find_package(PkgConfig REQUIRED)

pkg_check_modules(BLUEZ bluez)

if(BLUEZ_FOUND)
    set(NET-RC_CPP SocketAddress.cpp config/ConfigAddress.cpp)

    set(NET-RC_H SocketAddress.h config/ConfigAddress.h)

    add_library(net-rc SHARED ${NET-RC_CPP} ${NET-RC_H})
    add_library(snodec::net-rc ALIAS net-rc)

    target_link_libraries(net-rc PUBLIC net ${BLUEZ_LIBRARIES})

    target_include_directories(
        net-rc
        PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )

    set_target_properties(
        net-rc PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                                        snodec-net-rc
    )

    install(
        TARGETS net-rc
        EXPORT snodec_net-rc_Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-rc
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/rc
        COMPONENT net-rc
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "cmake" EXCLUDE
        PATTERN "stream" EXCLUDE
        PATTERN "dgram" EXCLUDE
        PATTERN "phy" EXCLUDE
    )

    install(
        EXPORT snodec_net-rc_Targets
        FILE snodec_net-rc_Targets.cmake
        NAMESPACE snodec::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
        COMPONENT net-rc
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
