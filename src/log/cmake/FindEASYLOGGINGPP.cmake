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
#
# CMake module for Easylogging++ logging library
#
# Defines ${EASYLOGGINGPP_INCLUDE_DIR}
#
# If ${EASYLOGGINGPP_USE_STATIC_LIBS} is ON then static libs are searched.
# In these cases ${EASYLOGGINGPP_LIBRARY} is also defined
#
# (c) 2017-2018 Zuhd Web Services
#
# https://github.com/zuhd-org/easyloggingpp
# https://zuhd.org
# https://muflihun.com
#

include(FindPackageHandleStandardArgs)

execute_process(COMMAND ${CMAKE_COMMAND} -E echo_append "-- Easylogging++: Searching...")
set(EASYLOGGINGPP_PATHS ${EASYLOGGINGPP_ROOT} $ENV{EASYLOGGINGPP_ROOT})

find_path(EASYLOGGINGPP_INCLUDE_DIR
        easylogging++.h
        PATH_SUFFIXES include
        PATHS ${EASYLOGGINGPP_PATHS}
)

find_package_handle_standard_args(EASYLOGGINGPP REQUIRED_VARS EASYLOGGINGPP_INCLUDE_DIR)

if (EASYLOGGINGPP_INCLUDE_DIR)
    execute_process(COMMAND ${CMAKE_COMMAND} -E echo "found")
endif()

if (EASYLOGGINGPP_USE_STATIC_LIBS)
    message ("-- Easylogging++: Static linking is preferred")
    find_library(EASYLOGGINGPP_LIBRARY
        NAMES libeasyloggingpp.a libeasyloggingpp.dylib libeasyloggingpp
        HINTS "${CMAKE_PREFIX_PATH}/lib"
    )
elseif (EASYLOGGINGPP_USE_SHARED_LIBS)
    message ("-- Easylogging++: Dynamic linking is preferred")
    find_library(EASYLOGGINGPP_LIBRARY
        NAMES libeasyloggingpp.dylib libeasyloggingpp libeasyloggingpp.a
        HINTS "${CMAKE_PREFIX_PATH}/lib"
    )
endif()
