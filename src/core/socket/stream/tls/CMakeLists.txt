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

find_package(OpenSSL REQUIRED)

if(NOT OpenSSL_FOUND)
    message(FATAL_TRACE " libssl-dev not found:\n"
            "   libssl-dev is mandatory for SNode.C.\n" "   Thus, install it!"
    )
else(NOT OpenSSL_FOUND)

    set(CORE-SOCKET_STREAM-TLS_CPP
        ssl_utils.cpp TLSHandshake.cpp TLSShutdown.cpp system/ssl.cpp
        SocketReader.cpp SocketWriter.cpp
    )

    set(CORE-SOCKET_STREAM-TLS_H
        SocketAcceptor.h
        SocketAcceptor.hpp
        SocketConnection.h
        SocketConnection.hpp
        SocketConnector.h
        SocketConnector.hpp
        SocketReader.h
        SocketWriter.h
        TLSHandshake.h
        TLSShutdown.h
        ssl_utils.h
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
               "$<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIRS}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )

    target_link_libraries(
        core-socket-stream-tls PUBLIC core-socket-stream ${OPENSSL_LIBRARIES}
    )

    set_target_properties(
        core-socket-stream-tls
        PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
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
