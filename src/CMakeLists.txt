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

project(snode.c-code)

if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12.2)
        message(STATUS "GNU compiler version " ${CMAKE_CXX_COMPILER_VERSION}
                       " >= 12.2: Good"
        )
    else()
        message(
            FATAL_ERROR "GNU compiler version " ${CMAKE_CXX_COMPILER_VERSION}
                        ": To old! Version required: >= 12.2"
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)
        message(STATUS "CLANG compiler version " ${CMAKE_CXX_COMPILER_VERSION}
                       " >= 13.0: Good"
        )
    else()
        message(
            FATAL_ERROR "CLANG compiler version " ${CMAKE_CXX_COMPILER_VERSION}
                        ": To old! Version required: >= 13.0"
        )
    endif()
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AppendSourceFileConfig)
include(GetAllTargetsDependencies)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(iwyu)

set(SNODEC_INTREE_BUILD "YES")

if("${SNODEC_INTREE_BUILD}" STREQUAL "YES")
    add_compile_definitions(SNODEC_INTREE_BUILD)
endif("${SNODEC_INTREE_BUILD}" STREQUAL "YES")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wconversion
    -Wpedantic
    -Wconversion
    -Wuninitialized
    -Wunreachable-code
    -Wfloat-equal
    -pedantic-errors
    -fexec-charset=UTF-8
    -Wno-shadow # yes we do
    -Wno-psabi # needed for RaspberryPi
    -Wno-used-but-marked-unused
    $<$<CXX_COMPILER_ID:Clang>:-Weverything>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-c++98-compat-pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-exit-time-destructors>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-global-constructors>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-shadow-field>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-padded>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-documentation-unknown-command>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,16.0>>:-Wno-unsafe-buffer-usage>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,18.0>>:-Wno-switch-default>
    $<$<OR:$<STREQUAL:${CMAKE_SYSTEM_NAME},Android>,$<BOOL:$ENV{TERMUX_ARCH}>>:-Wno-deprecated-declarations>
)

add_link_options(LINKER:--as-needed)

# add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
# add_link_options(-fsanitize=address,undefined)
# export LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/15/libasan.s

option(SNODEC_DISABLE_LOGLEVEL_LOGGING "Disable LOG() and PLOG() logging")
option(SNODEC_DISABLE_VERBOSE_LOGGING "Disable VLOG() logging")

if(SNODEC_DISABLE_LOGLEVEL_LOGGING)
    add_compile_definitions(ELPP_DISABLE_TRACE_LOGS)
    add_compile_definitions(ELPP_DISABLE_DEBUG_LOGS)
    add_compile_definitions(ELPP_DISABLE_INFO_LOGS)
    add_compile_definitions(ELPP_DISABLE_WARNING_LOGS)
    add_compile_definitions(ELPP_DISABLE_ERROR_LOGS)
    add_compile_definitions(ELPP_DISABLE_FATAL_LOGS)
endif(SNODEC_DISABLE_LOGLEVEL_LOGGING)

if(SNODEC_DISABLE_VERBOSE_LOGGING)
    add_compile_definitions(ELPP_DISABLE_VERBOSE_LOGS)
endif(SNODEC_DISABLE_VERBOSE_LOGGING)

add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(net)
add_subdirectory(web)
add_subdirectory(express)
add_subdirectory(database)
add_subdirectory(iot)
add_subdirectory(apps)

get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES log)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES utils)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES core)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES net)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES web)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES express)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES database)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES iot)

set(SUPPORTED_COMPONENTS
    core
    core-socket-stream-legacy
    core-socket-stream-tls
    net
    net-in-stream-legacy
    net-in6-stream-legacy
    net-un-stream-legacy
    net-in-stream-tls
    net-in6-stream-tls
    net-un-stream-tls
    ${NET-RC-STREAM-LEGACY}
    ${NET-RC-STREAM-TLS}
    ${NET-L2-STREAM-LEGACY}
    ${NET-L2-STREAM-TLS}
    net-un-dgram
    db-mariadb
    http
    http-server
    http-client
    http-server-express
    http-server-express-legacy-in
    http-server-express-legacy-in6
    http-server-express-legacy-rc
    http-server-express-legacy-un
    http-server-express-tls-in
    http-server-express-tls-in6
    http-server-express-tls-rc
    http-server-express-tls-un
    websocket-server
    websocket-client
    mqtt
    mqtt-fast
    mqtt-server
    mqtt-client
    mqtt-server-websocket
    mqtt-client-websocket
)
string(JOIN "\n    " SUPPORTED_COMPONENTS ${SUPPORTED_COMPONENTS})

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    NO_SET_AND_CHECK_MACRO
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/snodecConfigVersion.cmake
    VERSION
        "${SNode.C_VERSION_MAJOR}.${SNode.C_VERSION_MINOR}.${SNode.C_VERSION_PATCH}"
    COMPATIBILITY SameMinorVersion
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/snodecConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT "core"
)
