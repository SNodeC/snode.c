# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
snodec_add_test(McpServerTest McpServerTest.cpp)
target_include_directories(McpServerTest PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(
    McpServerTest PRIVATE snodec::mcp-server snodec-test-support
)
set_tests_properties(McpServerTest PROPERTIES LABELS "unit;mcp;schema;dispatch")
