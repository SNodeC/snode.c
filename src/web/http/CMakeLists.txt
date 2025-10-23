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

set(HTTP_COMPILE_DIR snode.c/http)
set(HTTP_COMPILE_LIBDIR ${CMAKE_BINARY_DIR}/${HTTP_COMPILE_DIR})
set(HTTP_UPGRADE_COMPILE_LIBDIR ${HTTP_COMPILE_LIBDIR}/upgrade)

set(HTTP_INSTALL_DIR snode.c/web/http)
set(HTTP_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/${HTTP_INSTALL_DIR})
set(HTTP_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/${HTTP_INSTALL_DIR})
set(HTTP_UPGRADE_INSTALL_LIBDIR ${HTTP_INSTALL_LIBDIR}/upgrade)
set(HTTP_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${HTTP_INSTALL_LIBDIR})

pkg_check_modules(LIBMAGIC libmagic)

set(HTTP_CPP
    CiStringMap.cpp
    ContentDecoder.cpp
    MimeTypes.cpp
    Parser.cpp
    StatusCodes.cpp
    http_utils.cpp
    decoder/Chunked.cpp
    decoder/Fields.cpp
    decoder/HTTP10Response.cpp
    decoder/Identity.cpp
)

set(HTTP_H
    CiStringMap.h
    ConnectionState.h
    ContentDecoder.h
    ContentType.h
    CookieOptions.h
    MimeTypes.h
    Parser.h
    SocketContextFactory.h
    SocketContextUpgrade.h
    SocketContextUpgradeFactory.h
    SocketContextUpgradeFactory.hpp
    SocketContextUpgradeFactorySelector.h
    SocketContextUpgradeFactorySelector.hpp
    decoder/Chunked.h
    decoder/Fields.h
    decoder/HTTP10Response.h
    decoder/Identity.h
    legacy/in/Client.h
    legacy/in/EventSource.h
    legacy/in/Server.h
    legacy/in6/Client.h
    legacy/in6/Server.h
    legacy/rc/Client.h
    legacy/rc/Server.h
    legacy/un/Client.h
    legacy/un/Server.h
    tls/in/Client.h
    tls/in/Server.h
    tls/in6/Client.h
    tls/in6/Server.h
    tls/rc/Client.h
    tls/rc/Server.h
    tls/un/Client.h
    tls/un/Server.h
)

add_library(http SHARED ${HTTP_CPP} ${HTTP_H})
add_library(snodec::http ALIAS http)

if(LIBMAGIC_FOUND)
    target_compile_definitions(http PUBLIC HAS_LIBMAGIC)
else(LIBMAGIC_FOUND)
    message(
        WARNING
            " libmagic-dev not found:\n"
            "    libmagic is used for the automatic detection of Mime-type files.\n"
            "    SNode.C has some built-in knowledge of mime types which, compared to libmime,\n"
            "    is not as reliable. So you should install libmagic-dev for better mime type detection."
    )
endif(LIBMAGIC_FOUND)

target_include_directories(
    http
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${LIBMAGIC_INCLUDE_DIRS}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(http PUBLIC core-socket-stream ${LIBMAGIC_LIBRARIES})

set_target_properties(
    http
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-http
               LIBRARY_OUTPUT_DIRECTORY ${HTTP_COMPILE_LIBDIR}
               HTTP_INSTALL_RPATH ${HTTP_INSTALL_RPATH}
               HTTP_COMPILE_LIBDIR "${HTTP_COMPILE_LIBDIR}"
               HTTP_INSTALL_LIBDIR "${HTTP_INSTALL_LIBDIR}"
               HTTP_UPGRADE_COMPILE_LIBDIR ${HTTP_UPGRADE_COMPILE_LIBDIR}
               HTTP_UPGRADE_INSTALL_LIBDIR ${HTTP_UPGRADE_INSTALL_LIBDIR}
)

install(
    TARGETS http
    EXPORT snodec_http_Targets
    LIBRARY DESTINATION ${HTTP_INSTALL_LIBDIR} COMPONENT http
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${HTTP_INSTALL_INCLUDEDIR}
    COMPONENT http
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "server" EXCLUDE
    PATTERN "client" EXCLUDE
    PATTERN "legacy" EXCLUDE
    PATTERN "tls" EXCLUDE
)

install(
    EXPORT snodec_http_Targets
    FILE snodec_http_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT http
)

add_subdirectory(server)
add_subdirectory(client)

configure_file(
    cmake/snodec_http_Targets_Config.cmake.in snodec_http_Targets_Config.cmake
    @ONLY
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/snodec_http_Targets_Config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT http
)
