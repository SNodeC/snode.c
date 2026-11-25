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
         COMMAND ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/stdio_test.py
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
             COMMAND ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/http_test.py
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

add_test(NAME McpHttpReplayTest
         COMMAND ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/replay_test.py
                 $<TARGET_FILE:McpHttpFixture>
)
set_tests_properties(
    McpHttpReplayTest PROPERTIES LABELS "mcp;component;http;resumability"
                                 TIMEOUT 30
)

add_test(
    NAME McpClientTransportTest
    COMMAND
        ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/client_test.py
        $<TARGET_FILE:snodec-mcp-client> $<TARGET_FILE:McpStdioFixture>
        $<TARGET_FILE:McpHttpFixture> $<TARGET_FILE:McpClientFixture>
)
set_tests_properties(
    McpClientTransportTest PROPERTIES LABELS "mcp;client;component;stdio;http"
                                      TIMEOUT 120
)

add_executable(McpClientFixture ClientFixture.cpp)
target_include_directories(McpClientFixture PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(McpClientFixture PRIVATE mcp-client mcp-client-http)

snodec_add_test(McpClientProcessTest ClientProcessTest.cpp)
target_include_directories(McpClientProcessTest PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(
    McpClientProcessTest PRIVATE mcp-client snodec-test-support
)
set_tests_properties(
    McpClientProcessTest
    PROPERTIES LABELS "component;mcp;client;stdio;lifecycle" TIMEOUT 10
)

if(SNODEC_MCP_SDK_ROOT)
    add_test(
        NAME McpClientSdkTest
        COMMAND
            ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/client_sdk_test.py
            ${MCP_NODE} ${SNODEC_MCP_SDK_ROOT} $<TARGET_FILE:snodec-mcp-client>
    )
    set_tests_properties(
        McpClientSdkTest PROPERTIES LABELS "mcp;client;component;sdk" TIMEOUT
                                    120
    )
endif()

add_test(NAME McpClientHandshakeTest
         COMMAND ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/handshake_test.py
                 $<TARGET_FILE:snodec-mcp-client> $<TARGET_FILE:McpHttpFixture>
)
set_tests_properties(
    McpClientHandshakeTest
    PROPERTIES LABELS "component;mcp;client;http;lifecycle" TIMEOUT 20
)

add_test(
    NAME McpTaskStdioLingerTest
    COMMAND ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/task_linger_test.py
            $<TARGET_FILE:McpStdioFixture>
)
set_tests_properties(
    McpTaskStdioLingerTest PROPERTIES LABELS "mcp;component;stdio;tasks"
                                      TIMEOUT 10
)

if(TARGET snodec-mcp-client AND TARGET snodec-dev-stdio)
    add_test(
        NAME McpClientGracefulShutdownTest
        COMMAND
            ${MCP_PYTHON} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/graceful_shutdown_test.py
            $<TARGET_FILE:snodec-mcp-client> $<TARGET_FILE:snodec-dev-stdio>
    )
    set_tests_properties(
        McpClientGracefulShutdownTest
        PROPERTIES LABELS "mcp;component;client;stdio;lifecycle" TIMEOUT 15
    )
endif()

add_test(NAME McpClientHttpReuseTest
         COMMAND ${MCP_PYTHON} -B ${CMAKE_CURRENT_SOURCE_DIR}/reuse_test.py
                 $<TARGET_FILE:snodec-mcp-client> $<TARGET_FILE:McpHttpFixture>
)
set_tests_properties(
    McpClientHttpReuseTest PROPERTIES LABELS "mcp;component;client;http"
                                      TIMEOUT 25
)
