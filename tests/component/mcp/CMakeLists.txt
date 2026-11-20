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
