# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
snodec_add_test(McpServerTest McpServerTest.cpp)
target_include_directories(McpServerTest PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(
    McpServerTest PRIVATE snodec::mcp-server snodec-test-support
)
set_tests_properties(McpServerTest PROPERTIES LABELS "unit;mcp;schema;dispatch")

# Compile positive controls before rejecting all six mismatched result kinds.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
foreach(reply Tool Resource Prompt)
    foreach(result Tool Resource Prompt)
        try_compile(
            accepts_${reply}_${result}
            "${CMAKE_CURRENT_BINARY_DIR}/reply-${reply}-${result}"
            SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ReplyTypeCheck.cpp"
            CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=20"
            COMPILE_DEFINITIONS
                "-I${CMAKE_SOURCE_DIR}/src" "-DMCP_REPLY=${reply}Reply"
                "-DMCP_RESULT=${result}Result"
            OUTPUT_VARIABLE reply_output
        )
        if(reply STREQUAL result AND NOT accepts_${reply}_${result})
            message(
                FATAL_ERROR
                    "MCP positive compile control failed: ${reply_output}"
            )
        elseif(NOT reply STREQUAL result AND accepts_${reply}_${result})
            message(
                FATAL_ERROR
                    "MCP accepted mismatched ${reply}Reply/${result}Result"
            )
        endif()
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/reply-${reply}-${result}.log"
             "${reply_output}"
        )
    endforeach()
endforeach()
message(STATUS "MCP Reply: 3 positive controls and 6 type rejections passed")

snodec_add_test(McpResultTest McpResultTest.cpp)
target_include_directories(McpResultTest PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(
    McpResultTest PRIVATE snodec::mcp-server snodec-test-support
)
set_tests_properties(McpResultTest PROPERTIES LABELS "unit;mcp;results")

snodec_add_test(McpContextTest McpContextTest.cpp)
target_include_directories(McpContextTest PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(
    McpContextTest PRIVATE snodec::mcp-server snodec-test-support
)
set_tests_properties(
    McpContextTest PROPERTIES LABELS "unit;mcp;cancellation;deadline"
)

snodec_add_test(McpRegistrationTest McpRegistrationTest.cpp)
target_include_directories(McpRegistrationTest PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(
    McpRegistrationTest PRIVATE snodec::mcp-server snodec-test-support
)
set_tests_properties(
    McpRegistrationTest PROPERTIES LABELS "unit;mcp;registration"
)

snodec_add_test(McpConfigTest McpConfigTest.cpp)
target_include_directories(McpConfigTest PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(
    McpConfigTest PRIVATE snodec::mcp-server snodec-test-support
)
set_tests_properties(McpConfigTest PROPERTIES LABELS "unit;mcp;config")

snodec_add_test(McpStdioEntryTest McpStdioEntryTest.cpp)
target_include_directories(McpStdioEntryTest PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(
    McpStdioEntryTest PRIVATE snodec::mcp-server-stream
                              snodec::net-stdio-stream snodec-test-support
)
set_tests_properties(McpStdioEntryTest PROPERTIES LABELS "unit;mcp;startup")
add_test(NAME McpStdioEntryWrongOrder COMMAND McpStdioEntryTest wrong-order)
set_tests_properties(
    McpStdioEntryWrongOrder PROPERTIES LABELS "unit;mcp;startup" TIMEOUT 5
)

snodec_add_test(McpClassTest McpClassTest.cpp)
target_include_directories(McpClassTest PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(
    McpClassTest PRIVATE snodec::mcp-server snodec-test-support
)
set_tests_properties(McpClassTest PROPERTIES LABELS "unit;mcp;registration")

if(TARGET snodec-dev-tools)
    snodec_add_test(McpSchemaTest McpSchemaTest.cpp)
    target_include_directories(McpSchemaTest PRIVATE ${PROJECT_SOURCE_DIR})
    target_compile_definitions(
        McpSchemaTest PRIVATE MCP_SOURCE_ROOT="${PROJECT_SOURCE_DIR}"
    )
    target_link_libraries(
        McpSchemaTest PRIVATE snodec-dev-tools snodec-test-support
    )
    set_tests_properties(McpSchemaTest PROPERTIES LABELS "unit;mcp;schema")
endif()
