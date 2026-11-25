# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

add_library(jsonrpc-server-websocket SHARED SubProtocol.cpp SubProtocol.h)
add_library(snodec::jsonrpc-server-websocket ALIAS jsonrpc-server-websocket)
target_link_libraries(jsonrpc-server-websocket PUBLIC jsonrpc websocket-server)
target_include_directories(
    jsonrpc-server-websocket PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                                    "$<INSTALL_INTERFACE:include/snode.c>"
)
set_target_properties(
    jsonrpc-server-websocket
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               INSTALL_RPATH "$ORIGIN;$ORIGIN/../web/http/upgrade"
               OUTPUT_NAME snodec-jsonrpc-server-websocket
)
install(
    TARGETS jsonrpc-server-websocket
    EXPORT snodec_jsonrpc-server-websocket_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/jsonrpc
            COMPONENT jsonrpc-server-websocket
)
install(
    FILES SubProtocol.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/jsonrpc/server
    COMPONENT jsonrpc-server-websocket
)
install(
    EXPORT snodec_jsonrpc-server-websocket_Targets
    FILE snodec_jsonrpc-server-websocket_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT jsonrpc-server-websocket
)
