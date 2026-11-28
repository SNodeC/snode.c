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

find_package(nlohmann_json 3.11 REQUIRED)
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
    mcp SHARED
    Results.cpp
    Schema.cpp
    Types.cpp
    Utilities.cpp
    Decode.cpp
    Headers.cpp
    ClientInputs.cpp
    detail/Tasks.cpp
    detail/Tasks.h
)
add_library(snodec::mcp ALIAS mcp)
target_compile_features(mcp PUBLIC cxx_std_20)
target_link_libraries(
    mcp
    PUBLIC nlohmann_json::nlohmann_json
    PRIVATE nlohmann_json_schema_validator PkgConfig::MCP_PCRE2 utils core
            jsonrpc
)
target_include_directories(
    mcp SYSTEM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/json-schema-validator/src"
)
target_include_directories(
    mcp PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
)
set_target_properties(
    mcp
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_MCP_SOVERSION}
               OUTPUT_NAME snodec-mcp
               INSTALL_RPATH "$ORIGIN;$ORIGIN/../jsonrpc;$ORIGIN/../.."
)
install(
    TARGETS mcp
    EXPORT snodec_mcp_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/mcp COMPONENT mcp
)
install(
    FILES Types.h Results.h Protocol.h Schema.h ClientInputs.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/mcp
    COMPONENT mcp
)
install(
    EXPORT snodec_mcp_Targets
    FILE snodec_mcp_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mcp
)
install(
    FILES cmake/snodec_mcp_Targets_Config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mcp
)
install(
    FILES detail/Headers.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/mcp/detail
    COMPONENT mcp
)
install(
    FILES json-schema-validator/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/snodec/mcp
    RENAME json-schema-validator.LICENSE
    COMPONENT mcp
)
install(
    FILES json-schema-validator/src/metaschemas/LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/snodec/mcp
    RENAME json-schema-metaschemas.LICENSE
    COMPONENT mcp
)
add_subdirectory(server)

add_subdirectory(client)
