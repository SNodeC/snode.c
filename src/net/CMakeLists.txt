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

set(NET_CPP
    phy/PhysicalSocketOption.cpp
    config/ConfigInstance.cpp
    config/ConfigConnection.cpp
    config/ConfigLegacy.cpp
    config/ConfigPhysicalSocket.cpp
    config/ConfigPhysicalSocketClient.cpp
    config/ConfigPhysicalSocketServer.cpp
    config/ConfigSection.cpp
    config/ConfigTls.cpp
    config/ConfigTlsServer.cpp
    config/ConfigTlsClient.cpp
)

set(NET_H
    SocketAddress.h
    SocketAddress.hpp
    config/ConfigAddress.h
    config/ConfigAddress.hpp
    config/ConfigAddressBase.h
    config/ConfigAddressBase.hpp
    config/ConfigAddressReverse.hpp
    config/ConfigAddressLocal.h
    config/ConfigAddressLocal.hpp
    config/ConfigAddressRemote.h
    config/ConfigAddressRemote.hpp
    config/ConfigAddressReverse.h
    config/ConfigAddressReverse.hpp
    config/ConfigInstance.h
    config/ConfigConnection.h
    config/ConfigLegacy.h
    config/ConfigPhysicalSocket.h
    config/ConfigPhysicalSocketClient.h
    config/ConfigPhysicalSocketServer.h
    config/ConfigSection.h
    config/ConfigSection.hpp
    config/ConfigTls.h
    config/ConfigTlsServer.h
    config/ConfigTlsClient.h
    config/stream/ConfigSocketClient.h
    config/stream/ConfigSocketClient.hpp
    config/stream/ConfigSocketServer.h
    config/stream/ConfigSocketServer.hpp
    config/stream/legacy/ConfigSocketClient.h
    config/stream/legacy/ConfigSocketClient.hpp
    config/stream/legacy/ConfigSocketServer.h
    config/stream/legacy/ConfigSocketServer.hpp
    config/stream/tls/ConfigSocketClient.h
    config/stream/tls/ConfigSocketClient.hpp
    config/stream/tls/ConfigSocketServer.h
    config/stream/tls/ConfigSocketServer.hpp
    phy/PhysicalSocket.h
    phy/PhysicalSocket.hpp
    phy/PhysicalSocketOption.h
    phy/dgram/PeerSocket.h
    phy/dgram/PeerSocket.hpp
    phy/stream/PhysicalSocket.h
    phy/stream/PhysicalSocket.hpp
    phy/stream/PhysicalSocketClient.h
    phy/stream/PhysicalSocketClient.hpp
    phy/stream/PhysicalSocketServer.h
    phy/stream/PhysicalSocketServer.hpp
)

append_source_file_config(
    config/ConfigConnection.cpp READ_BLOCKSIZE "Read block size in bytes" 16384
)
append_source_file_config(
    config/ConfigConnection.cpp WRITE_BLOCKSIZE "Write block size in bytes"
    16384
)
append_source_file_config(
    config/ConfigConnection.cpp READ_TIMEOUT
    "Read inactivity timeout in seconds" 60
)
append_source_file_config(
    config/ConfigConnection.cpp WRITE_TIMEOUT
    "Write inactivity timeout in seconds" 60
)
append_source_file_config(
    config/ConfigConnection.cpp TERMINATE_TIMEOUT
    "Shutdown inactivity timeout in seconds" 1
)

append_source_file_config(
    config/ConfigPhysicalSocketServer.cpp ACCEPTS_PER_TICK "Accepts per tick" 1
)
append_source_file_config(
    config/ConfigPhysicalSocketServer.cpp BACKLOG "Listen backlog" 5
)

append_source_file_config(
    config/ConfigPhysicalSocket.cpp RETRY "Retry listen and connect" false
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp RETRY_ON_FATAL "Retry also on fatal errors"
    false
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp RETRY_TIMEOUT "Retry interval in seconds" 1
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp RETRY_TRIES "Upper limit of retry tries" 0
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp RETRY_BASE "Base of exponential backoff"
    1.8
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp RETRY_JITTER
    "Jitter of retry timeout in percent" 0
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp RETRY_LIMIT "Upper limit of retry timeout"
    0
)

append_source_file_config(
    config/ConfigPhysicalSocketServer.cpp ACCEPT_TIMEOUT
    "Accept inactivity timeout in seconds" 0
)

append_source_file_config(
    config/ConfigPhysicalSocketClient.cpp RECONNECT
    "Reconnect after disconnect" false
)
append_source_file_config(
    config/ConfigPhysicalSocketClient.cpp RECONNECT_TIME
    "Reconnect time in seconds" 1
)
append_source_file_config(
    config/ConfigPhysicalSocketClient.cpp CONNECT_TIMEOUT
    "Connect timeout in seconds" 10
)

append_source_file_config(
    config/ConfigTls.cpp TLS_INIT_TIMEOUT
    "SSL/TLS initial handshake timeout in seconds" 10
)
append_source_file_config(
    config/ConfigTls.cpp TLS_SHUTDOWN_TIMEOUT
    "SSL/TLS teardown timeout in seconds" 1
)

add_library(net SHARED ${NET_CPP} ${NET_H})
add_library(snodec::net ALIAS net)

target_include_directories(
    net PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net PUBLIC utils)

set_target_properties(
    net
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-net
)

install(
    TARGETS net
    EXPORT snodec_net_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net
    COMPONENT net
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "in" EXCLUDE
    PATTERN "in6" EXCLUDE
    PATTERN "l2" EXCLUDE
    PATTERN "rc" EXCLUDE
    PATTERN "un" EXCLUDE
)

install(
    EXPORT snodec_net_Targets
    FILE snodec_net_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net
)

add_subdirectory(in)
add_subdirectory(in6)
add_subdirectory(l2)
add_subdirectory(rc)
add_subdirectory(un)
