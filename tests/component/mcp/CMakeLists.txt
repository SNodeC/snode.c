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
