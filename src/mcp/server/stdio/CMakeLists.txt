# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
add_library(
    mcp-server-stdio SHARED
    ../stream/SocketContext.cpp ../stream/SocketContext.h
    ../stream/SocketContextFactory.cpp ../stream/SocketContextFactory.h
)
add_library(snodec::mcp-server-stdio ALIAS mcp-server-stdio)
target_link_libraries(mcp-server-stdio PUBLIC mcp-server core-socket-stream)
set_target_properties(
    mcp-server-stdio
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_MCP_SOVERSION}
               OUTPUT_NAME snodec-mcp-server-stdio
)
install(
    TARGETS mcp-server-stdio
    EXPORT snodec_mcp-server-stdio_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/mcp
            COMPONENT mcp-server-stdio
)
install(
    FILES ../stream/SocketContext.h ../stream/SocketContextFactory.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/mcp/server/stream
    COMPONENT mcp-server-stdio
)
install(
    EXPORT snodec_mcp-server-stdio_Targets
    FILE snodec_mcp-server-stdio_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mcp-server-stdio
)
