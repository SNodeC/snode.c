# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
find_program(MCP_PYTHON NAMES python3 REQUIRED)
add_executable(McpStdioFixture StdioFixture.cpp)
target_link_libraries(McpStdioFixture PRIVATE snodec::mcp-server-stdio)
add_test(NAME McpStdioTest
         COMMAND ${MCP_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/stdio_test.py
                 $<TARGET_FILE:McpStdioFixture>
)
set_tests_properties(
    McpStdioTest PROPERTIES LABELS "mcp;component;stdio" TIMEOUT 20
)
