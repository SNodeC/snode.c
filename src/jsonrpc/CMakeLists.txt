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

find_package(nlohmann_json 3.11 REQUIRED)

add_library(
    jsonrpc SHARED
    JsonRpc.cpp
    JsonRpc.h
    JsonRpcContext.cpp
    JsonRpcContext.h
    SocketContext.cpp
    SocketContext.h
    SubProtocol.h
    SubProtocol.hpp
    messages/Request.cpp
    messages/Request.h
    messages/Notification.cpp
    messages/Notification.h
    messages/Result.cpp
    messages/Result.h
    messages/Error.cpp
    messages/Error.h
    messages/ErrorCode.h
    messages/Id.h
)
add_library(snodec::jsonrpc ALIAS jsonrpc)
target_compile_features(jsonrpc PUBLIC cxx_std_20)
target_include_directories(
    jsonrpc PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                   "$<INSTALL_INTERFACE:include/snode.c>"
)
target_link_libraries(
    jsonrpc PUBLIC core core-socket-stream nlohmann_json::nlohmann_json
)
set_target_properties(
    jsonrpc
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               INSTALL_RPATH "$ORIGIN"
               OUTPUT_NAME snodec-jsonrpc
)
install(
    TARGETS jsonrpc
    EXPORT snodec_jsonrpc_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/jsonrpc
            COMPONENT jsonrpc
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/jsonrpc
    COMPONENT jsonrpc
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "server" EXCLUDE
    PATTERN "client" EXCLUDE
)
install(
    EXPORT snodec_jsonrpc_Targets
    FILE snodec_jsonrpc_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT jsonrpc
)
install(
    FILES cmake/snodec_jsonrpc_Targets_Config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT jsonrpc
)

add_subdirectory(server)
add_subdirectory(client)
