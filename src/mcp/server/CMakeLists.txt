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

find_package(PkgConfig REQUIRED)
pkg_check_modules(MCP_JSON REQUIRED nlohmann_json>=3.11)
pkg_check_modules(MCP_PCRE2 REQUIRED IMPORTED_TARGET libpcre2-8)

set(MCP_SAVED_IWYU "${CMAKE_CXX_INCLUDE_WHAT_YOU_USE}")
unset(CMAKE_CXX_INCLUDE_WHAT_YOU_USE)
set(JSON_VALIDATOR_INSTALL
    OFF
    CACHE BOOL "" FORCE
)
set(JSON_VALIDATOR_BUILD_TESTS
    OFF
    CACHE BOOL "" FORCE
)
set(JSON_VALIDATOR_BUILD_EXAMPLES
    OFF
    CACHE BOOL "" FORCE
)
set(JSON_VALIDATOR_SHARED_LIBS
    OFF
    CACHE BOOL "" FORCE
)
set(JSON_VALIDATOR_USE_PCRE2
    ON
    CACHE BOOL "" FORCE
)
add_subdirectory(json-schema-validator)
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${MCP_SAVED_IWYU}")
set_target_properties(
    nlohmann_json_schema_validator PROPERTIES POSITION_INDEPENDENT_CODE ON
)
target_compile_options(
    nlohmann_json_schema_validator
    PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-error>
            $<$<CXX_COMPILER_ID:Clang>:-Wno-error>
)

add_library(
    mcp-server SHARED
    Mcp.cpp
    Mcp.h
    Server.cpp
    Server.h
    detail/UriTemplate.cpp
    Config.cpp
    Config.h
    stream/SocketContext.cpp
    stream/SocketContext.h
    stream/SocketContextFactory.cpp
    stream/SocketContextFactory.h
)
add_library(snodec::mcp-server ALIAS mcp-server)
target_compile_features(mcp-server PUBLIC cxx_std_20)
target_link_libraries(
    mcp-server
    PUBLIC jsonrpc core-socket-stream
    PRIVATE nlohmann_json_schema_validator PkgConfig::MCP_PCRE2
)
target_include_directories(
    mcp-server SYSTEM
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/json-schema-validator/src"
)
target_include_directories(
    mcp-server
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>" ${MCP_JSON_INCLUDE_DIRS}
)
set_target_properties(
    mcp-server
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_MCP_SOVERSION}
               INSTALL_RPATH "$ORIGIN/../jsonrpc"
               OUTPUT_NAME snodec-mcp-server
)
install(
    TARGETS mcp-server
    EXPORT snodec_mcp-server_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/mcp
            COMPONENT mcp-server
)
install(
    FILES Server.h
          Config.h
          Types.h
          Results.h
          Reply.h
          Request.h
          Tool.h
          Resource.h
          ResourceTemplate.h
          Prompt.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/mcp/server
    COMPONENT mcp-server
)
install(
    FILES stream/SocketContext.h stream/SocketContextFactory.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/mcp/server/stream
    COMPONENT mcp-server
)
install(
    FILES json-schema-validator/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/snodec/mcp-server
    RENAME json-schema-validator.LICENSE
    COMPONENT mcp-server
)
install(
    FILES json-schema-validator/src/metaschemas/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/snodec/mcp-server
    RENAME json-schema-metaschemas.LICENSE
    COMPONENT mcp-server
)
install(
    EXPORT snodec_mcp-server_Targets
    FILE snodec_mcp-server_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mcp-server
)

add_subdirectory(http)
