# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
configure_file(config.h.in mcp-config.h @ONLY)
add_library(
    snodec-dev-tools STATIC
    Tools.cpp
    Tools.h
    FindTool.cpp
    FindTool.h
    ReadTool.cpp
    ReadTool.h
    SkeletonPreviewTool.cpp
    SkeletonPreviewTool.h
    HouseStyleResource.cpp
    HouseStyleResource.h
    ReviewPrompt.cpp
    ReviewPrompt.h
)
target_include_directories(snodec-dev-tools PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(snodec-dev-tools PUBLIC snodec::mcp-server)
foreach(transport IN ITEMS stdio http)
    add_executable(snodec-dev-${transport} main.cpp)
    target_link_libraries(
        snodec-dev-${transport} PRIVATE snodec::mcp-server-${transport}
                                        snodec-dev-tools
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
