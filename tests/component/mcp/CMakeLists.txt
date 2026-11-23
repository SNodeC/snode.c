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

find_program(MCP_PYTHON NAMES python3 REQUIRED)
add_executable(McpStdioFixture StdioFixture.cpp)
target_link_libraries(
    McpStdioFixture PRIVATE snodec::mcp-server snodec::net-stdio-stream
)
add_test(NAME McpStdioTest
         COMMAND ${MCP_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/stdio_test.py
                 $<TARGET_FILE:McpStdioFixture>
)
set_tests_properties(
    McpStdioTest PROPERTIES LABELS "mcp;component;stdio" TIMEOUT 20
)

find_program(MCP_CURL NAMES curl REQUIRED)
foreach(variant IN ITEMS Http Tls)
    add_executable(Mcp${variant}Fixture HttpFixture.cpp)
    target_link_libraries(Mcp${variant}Fixture PRIVATE snodec::mcp-server-http)
    if(variant STREQUAL "Tls")
        target_compile_definitions(Mcp${variant}Fixture PRIVATE MCP_TLS=1)
        target_link_libraries(
            Mcp${variant}Fixture PRIVATE snodec::net-in-stream-tls
        )
    else()
        target_link_libraries(
            Mcp${variant}Fixture PRIVATE snodec::net-in-stream-legacy
        )
    endif()
    add_test(NAME Mcp${variant}Test
             COMMAND ${MCP_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/http_test.py
                     $<TARGET_FILE:Mcp${variant}Fixture> ${MCP_CURL} ${variant}
    )
    set_tests_properties(
        Mcp${variant}Test PROPERTIES LABELS "mcp;component;http;curl" TIMEOUT
                                     30
    )
endforeach()

if(TARGET snodec-dev-stdio AND TARGET snodec-dev-http)
    foreach(transport IN ITEMS stdio http)
        add_test(
            NAME McpSourceSearch_${transport}
            COMMAND
                ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/search_test.py
                ${transport} $<TARGET_FILE:snodec-dev-${transport}> ${MCP_CURL}
                ${CMAKE_SOURCE_DIR}/src
        )
        set_tests_properties(
            McpSourceSearch_${transport}
            PROPERTIES LABELS "mcp;component;development;timing" TIMEOUT 35
        )
    endforeach()
    add_test(
        NAME McpReadTest
        COMMAND
            ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/read_test.py
            $<TARGET_FILE:snodec-dev-stdio> $<TARGET_FILE:snodec-dev-http>
            ${MCP_CURL} ${CMAKE_SOURCE_DIR}/src
    )
    set_tests_properties(
        McpReadTest PROPERTIES LABELS "mcp;component;development" TIMEOUT 30
    )
    set(SNODEC_MCP_SDK_ROOT
        ""
        CACHE
            PATH
            "Official @modelcontextprotocol/client package directory for SDK regression tests"
    )
    if(SNODEC_MCP_SDK_ROOT)
        find_program(MCP_NODE NAMES node REQUIRED)
        add_test(
            NAME McpReadSdkTest
            COMMAND
                ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/read_sdk_test.py
                ${MCP_NODE} ${SNODEC_MCP_SDK_ROOT}
                $<TARGET_FILE:snodec-dev-stdio> $<TARGET_FILE:snodec-dev-http>
                ${MCP_CURL} ${CMAKE_SOURCE_DIR}/src
        )
        set_tests_properties(
            McpReadSdkTest PROPERTIES LABELS "mcp;component;development;sdk"
                                      TIMEOUT 40
        )
    endif()
    add_test(
        NAME McpDevelopmentTest
        COMMAND
            ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/dev_test.py
            $<TARGET_FILE:snodec-dev-stdio> $<TARGET_FILE:snodec-dev-http>
            ${MCP_CURL} ${CMAKE_CXX_COMPILER}
            ${CMAKE_BINARY_DIR}/staged-installed-consumer/prefix/${CMAKE_INSTALL_LIBDIR}/cmake/snodec
            "${CMAKE_CXX_FLAGS}" "${CMAKE_EXE_LINKER_FLAGS}"
    )
    set_tests_properties(
        McpDevelopmentTest
        PROPERTIES LABELS "mcp;component;development" DEPENDS
                   StagedInstalledConsumerTest TIMEOUT 120
    )
endif()

add_test(
    NAME McpStdioBootstrapTest
    COMMAND
        ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/bootstrap_test.py
        $<TARGET_FILE:McpStdioFixture> $<TARGET_FILE:snodec-dev-stdio>
        ${CMAKE_SOURCE_DIR}/src
)
set_tests_properties(
    McpStdioBootstrapTest PROPERTIES LABELS "mcp;component;stdio;startup"
                                     TIMEOUT 40
)
add_test(NAME McpStdioIdleTest
         COMMAND ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/idle_test.py
                 $<TARGET_FILE:McpStdioFixture>
)
set_tests_properties(
    McpStdioIdleTest PROPERTIES LABELS "mcp;component;stdio;idle" TIMEOUT 80
)

add_test(
    NAME McpFeaturesTest
    COMMAND
        ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/features_test.py
        $<TARGET_FILE:McpStdioFixture> $<TARGET_FILE:McpHttpFixture>
        ${MCP_CURL}
)
set_tests_properties(
    McpFeaturesTest
    PROPERTIES LABELS "component;mcp;completion;subscriptions;mrtr" TIMEOUT 90
)

add_test(
    NAME McpElicitationTest
    COMMAND
        ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/elicitation_test.py
        $<TARGET_FILE:McpStdioFixture> $<TARGET_FILE:McpHttpFixture>
        ${MCP_CURL}
)
set_tests_properties(
    McpElicitationTest PROPERTIES LABELS "component;mcp;elicitation;stdio;http"
                                  TIMEOUT 45
)
