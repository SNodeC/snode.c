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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddSNodeCTest)

add_subdirectory(support)
add_subdirectory(unit)
add_subdirectory(component)
add_subdirectory(policy)

add_test(NAME StagedInstalledConsumerTest
         COMMAND ${CMAKE_COMMAND}
                 -DSNODEC_BUILD_DIR=${CMAKE_BINARY_DIR}
                 -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                 -P ${CMAKE_CURRENT_SOURCE_DIR}/StagedInstalledConsumerTest.cmake)
set_tests_properties(
    StagedInstalledConsumerTest
    PROPERTIES LABELS "install;consumer;codex;phase-a1-2" TIMEOUT 120
)

add_test(
    NAME CodexSourcePackageTest
    COMMAND
        ${CMAKE_COMMAND} -DSNODEC_BUILD_DIR=${CMAKE_BINARY_DIR}
        -DSNODEC_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DCMAKE_CPACK_COMMAND=${CMAKE_CPACK_COMMAND} -P
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexSourcePackageTest.cmake
)
set_tests_properties(
    CodexSourcePackageTest
    PROPERTIES LABELS "package;source-package;codex;phase-a1-0" TIMEOUT 360
)

add_test(
    NAME CodexBinaryPackageTest
    COMMAND
        ${CMAKE_COMMAND} -DSNODEC_BUILD_DIR=${CMAKE_BINARY_DIR}
        -DCMAKE_CPACK_COMMAND=${CMAKE_CPACK_COMMAND} -P
        ${CMAKE_CURRENT_SOURCE_DIR}/CodexBinaryPackageTest.cmake
)
set_tests_properties(
    CodexBinaryPackageTest
    PROPERTIES LABELS "package;binary-package;codex;phase-a1-1" TIMEOUT 120
)
