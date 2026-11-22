# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
add_library(
    mcp-server-stream SHARED SocketContext.cpp SocketContext.h
                             SocketContextFactory.cpp SocketContextFactory.h
)
add_library(snodec::mcp-server-stream ALIAS mcp-server-stream)
target_link_libraries(mcp-server-stream PUBLIC mcp-server core-socket-stream)
set_target_properties(
    mcp-server-stream
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_MCP_SOVERSION}
               OUTPUT_NAME snodec-mcp-server-stream
)
install(
    TARGETS mcp-server-stream
    EXPORT snodec_mcp-server-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/mcp
            COMPONENT mcp-server-stream
)
install(
    FILES SocketContext.h SocketContextFactory.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/mcp/server/stream
    COMPONENT mcp-server-stream
)
install(
    EXPORT snodec_mcp-server-stream_Targets
    FILE snodec_mcp-server-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mcp-server-stream
)
