# snode.c - a slim toolkit for network communication
# Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

cmake_minimum_required(VERSION 3.18)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AppendSourceFileConfig)

set(NET_CPP
    PhysicalSocketOption.cpp
    config/ConfigInstance.cpp
    config/ConfigConnection.cpp
    config/ConfigLegacy.cpp
    config/ConfigListen.cpp
    config/ConfigPhysicalSocket.cpp
    config/ConfigPhysicalSocketClient.cpp
    config/ConfigPhysicalSocketServer.cpp
    config/ConfigSection.cpp
    config/ConfigTls.cpp
    config/ConfigTlsServer.cpp
    config/ConfigTlsClient.cpp
)

set(NET_H
    PhysicalSocket.h
    PhysicalSocket.hpp
    PhysicalSocketOption.h
    SocketAddress.h
    SocketAddress.hpp
    config/ConfigAddress.h
    config/ConfigAddress.hpp
    config/ConfigAddressLocal.h
    config/ConfigAddressLocal.hpp
    config/ConfigAddressRemote.h
    config/ConfigAddressRemote.hpp
    config/ConfigInstance.h
    config/ConfigConnection.h
    config/ConfigLegacy.h
    config/ConfigListen.h
    config/ConfigPhysicalSocket.h
    config/ConfigPhysicalSocketClient.h
    config/ConfigPhysicalSocketServer.h
    config/ConfigSection.h
    config/ConfigSection.hpp
    config/ConfigTls.h
    config/ConfigTlsServer.h
    config/ConfigTlsClient.h
    dgram/PeerSocket.h
    dgram/PeerSocket.hpp
    stream/PhysicalSocket.h
    stream/PhysicalSocket.hpp
    stream/PhysicalClientSocket.h
    stream/PhysicalClientSocket.hpp
    stream/PhysicalServerSocket.h
    stream/PhysicalServerSocket.hpp
    stream/config/ConfigSocketClient.h
    stream/config/ConfigSocketClient.hpp
    stream/config/ConfigSocketServer.h
    stream/config/ConfigSocketServer.hpp
    stream/config/legacy/ConfigSocketClient.h
    stream/config/legacy/ConfigSocketClient.hpp
    stream/config/legacy/ConfigSocketServer.h
    stream/config/legacy/ConfigSocketServer.hpp
    stream/config/tls/ConfigSocketClient.h
    stream/config/tls/ConfigSocketClient.hpp
    stream/config/tls/ConfigSocketServer.h
    stream/config/tls/ConfigSocketServer.hpp
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
    config/ConfigListen.cpp ACCEPTS_PER_TICK "Accepts per tick" 1
)
append_source_file_config(config/ConfigListen.cpp BACKLOG "Listen backlog" 5)

append_source_file_config(
    config/ConfigPhysicalSocket.cpp REUSE_ADDRESS "Reuse address" \"false\"
    \"true\"
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp RECONNECT "Reconnect after disconnect"
    \"false\" \"true\"
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp RECONNECT_TIME "Reconnect time in seconds"
    1
)
append_source_file_config(
    config/ConfigPhysicalSocket.cpp RETRY "Retry listen and connect" \"false\"
    \"true\"
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
    net
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net PUBLIC snodec::core)

set_target_properties(
    net PROPERTIES SOVERSION ${SNODEC_SO_VERSION} OUTPUT_NAME snodec-net
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
