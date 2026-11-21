# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
configure_file(config.h.in mcp-config.h @ONLY)
foreach(transport IN ITEMS stdio http)
    add_executable(snodec-dev-${transport} main.cpp Tools.cpp Tools.h)
    target_include_directories(
        snodec-dev-${transport} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    )
    target_compile_features(snodec-dev-${transport} PRIVATE cxx_std_20)
    target_link_libraries(
        snodec-dev-${transport} PRIVATE snodec::mcp-server-${transport}
    )
    if(transport STREQUAL "stdio")
        target_compile_definitions(snodec-dev-${transport} PRIVATE MCP_STDIO=1)
    else()
        target_link_libraries(
            snodec-dev-${transport} PRIVATE snodec::net-in-stream-legacy
        )
    endif()
    set_target_properties(
        snodec-dev-${transport}
        PROPERTIES
            INSTALL_RPATH
            "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH};${CMAKE_INSTALL_FULL_LIBDIR}/snode.c/mcp"
    )
    install(TARGETS snodec-dev-${transport}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
    )
endforeach()

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/skill/snodec-development/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/snode.c/skills/snodec-development
    COMPONENT apps
)
