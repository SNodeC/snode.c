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
project(McpReplyTypeTest LANGUAGES CXX)

# Check three matching result kinds and six rejected mismatches.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
foreach(reply Tool Resource Prompt)
    foreach(result Tool Resource Prompt)
        try_compile(
            accepts_${reply}_${result}
            "${CMAKE_CURRENT_BINARY_DIR}/reply-${reply}-${result}"
            SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../ReplyTypeCheck.cpp"
            CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=20"
            COMPILE_DEFINITIONS
                "-I${SNODEC_SOURCE_DIR}/src" "-DMCP_REPLY=${reply}Reply"
                "-DMCP_RESULT=${result}Result"
            OUTPUT_VARIABLE reply_output
        )
        if(reply STREQUAL result AND NOT accepts_${reply}_${result})
            message(
                SEND_ERROR
                    "MCP positive compile control failed: ${reply_output}"
            )
        elseif(NOT reply STREQUAL result AND accepts_${reply}_${result})
            message(
                SEND_ERROR
                    "MCP accepted mismatched ${reply}Reply/${result}Result"
            )
        endif()
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/reply-${reply}-${result}.log"
             "${reply_output}"
        )
    endforeach()
endforeach()
