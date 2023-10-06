# snode.c - a slim toolkit for network communication Copyright (C) 2020, 2021,
# 2022, 2023 Volker Christian <me@vchrist.at>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

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

set(SNODEC_INTREE_BUILD "YES")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AppendSourceFileConfig)
include(GetAllTargetsDependencies)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(iwyu)

set(IO_MULTIPLEXERS "epoll;poll;select")

if("${IO_MULTIPLEXER}" STREQUAL "")
    list(SUBLIST IO_MULTIPLEXERS 0 1 IO_MULTIPLEXER)
endif("${IO_MULTIPLEXER}" STREQUAL "")

set(SNODEC_IO_MULTIPLEXER
    ${IO_MULTIPLEXER}
    CACHE STRING "I/O-Multiplexer to use"
)
set_property(CACHE SNODEC_IO_MULTIPLEXER PROPERTY STRINGS ${IO_MULTIPLEXERS})
message(STATUS "I/O-Multiplexer selected: '${SNODEC_IO_MULTIPLEXER}'")

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
    $<$<CXX_COMPILER_ID:Clang>:-Weverything>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-c++98-compat-pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-exit-time-destructors>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-global-constructors>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-shadow-field>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-padded>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-documentation-unknown-command>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,16.0>>:-Wno-unsafe-buffer-usage>
)

# https://stackoverflow.com/questions/11055569/linking-with-dynamic-library-with-dependencies
add_link_options(LINKER:--as-needed)

set(SNODEC_SOVERSION ${snode.c_VERSION_MAJOR})

add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(net)
add_subdirectory(web)
add_subdirectory(express)
add_subdirectory(database)
add_subdirectory(iot)
add_subdirectory(apps)

# @LIST_OF_ALL_TARGETS_DEPENDENCIES@ in Config.cmake.in will be replaced by
# ${LIST_OF_ALL_TARGETS_DEPENDENCIES}
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES log)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES utils)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES core)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES net)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES web)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES express)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES database)
get_all_targets_dependencies(SNODEC_LIST_OF_ALL_TARGETS_DEPENDENCIES iot)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/snodecConfigVersion.cmake
    VERSION "${snode.c_VERSION_MAJOR}.${snode.c_VERSION_MINOR}"
    COMPATIBILITY SameMajorVersion
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/snodecConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT "core"
)
