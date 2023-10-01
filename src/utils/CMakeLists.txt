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

set(UTILS_CPP
    Config.cpp
    Daemon.cpp
    Formatter.cpp
    PreserveErrno.cpp
    Random.cpp
    ResetToDefault.cpp
    Timeval.cpp
    Uuid.cpp
    base64.cpp
    sha1.cpp
    system/signal.cpp
    system/time.cpp
)

set(UTILS_H
    AttributeInjector.h
    CLI11.hpp
    Config.h
    Daemon.h
    Formatter.h
    PreserveErrno.h
    Random.h
    ResetToDefault.h
    Timeval.h
    Uuid.h
    Validators.hpp
    base64.h
    sha1.h
    system/signal.h
    system/time.h
)

append_source_file_config(
    config/Config.cpp GROUP_NAME
    "Group name of group used for config file management" snodec
)

add_library(utils SHARED ${UTILS_CPP} ${UTILS_H})
add_library(snodec::utils ALIAS utils)

target_include_directories(
    utils
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(utils PUBLIC snodec::logger)

set_target_properties(
    utils PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME snodec-utils
)

install(
    TARGETS utils
    EXPORT snodec_utils_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT utils
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/utils
    COMPONENT utils
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_utils_Targets
    FILE snodec_utils_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT utils
)
