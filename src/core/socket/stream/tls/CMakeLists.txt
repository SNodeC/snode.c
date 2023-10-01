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

find_package(OpenSSL)

if(NOT OpenSSL_FOUND)
    message(
        FATAL_ERROR
            " libssl-dev not found:\n"
            "   libssl-dev is mandatory for SNode.C.\n" "   Thus, install it!"
    )
else(NOT OpenSSL_FOUND)

    set(CORE-SOCKET_STREAM-TLS_CPP ssl_utils.cpp TLSHandshake.cpp
                                   TLSShutdown.cpp system/ssl.cpp
    )

    set(CORE-SOCKET_STREAM-TLS_H
        SocketAcceptor.h
        SocketAcceptor.hpp
        SocketConnection.h
        SocketConnection.hpp
        SocketConnector.h
        SocketConnector.hpp
        SocketReader.h
        SocketReader.hpp
        SocketWriter.h
        SocketWriter.hpp
        TLSHandshake.h
        TLSShutdown.h
        ssl_utils.h
        ssl_version.h
        system/ssl.h
    )

    add_library(
        core-socket-stream-tls SHARED ${CORE-SOCKET_STREAM-TLS_CPP}
                                      ${CORE-SOCKET_STREAM-TLS_H}
    )
    add_library(snodec::core-socket-stream-tls ALIAS core-socket-stream-tls)

    target_include_directories(
        core-socket-stream-tls
        PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
               "$<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIRS}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )

    target_link_libraries(
        core-socket-stream-tls PUBLIC core-socket-stream ${OPENSSL_LIBRARIES}
    )

    set_target_properties(
        core-socket-stream-tls
        PROPERTIES SOVERSION ${SNODEC_SO_VERSION} OUTPUT_NAME
                                                 snodec-core-socket-stream-tls
    )

    install(
        TARGETS core-socket-stream-tls
        EXPORT snodec_core-socket-stream-tls_Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT core-socket-stream-tls
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/socket/stream/tls
        COMPONENT core-socket-stream-tls
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "cmake" EXCLUDE
    )

    install(
        EXPORT snodec_core-socket-stream-tls_Targets
        FILE snodec_core-socket-stream-tls_Targets.cmake
        NAMESPACE snodec::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
        COMPONENT core-socket-stream-tls
    )

endif(NOT OpenSSL_FOUND)
