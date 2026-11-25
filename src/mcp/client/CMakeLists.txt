# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
add_library(mcp-client SHARED Mcp.cpp Mcp.h)
add_library(snodec::mcp-client ALIAS mcp-client)
target_compile_features(mcp-client PUBLIC cxx_std_20)
target_link_libraries(mcp-client PUBLIC mcp jsonrpc)
set_target_properties(
    mcp-client
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_MCP_SOVERSION}
               OUTPUT_NAME snodec-mcp-client
               INSTALL_RPATH "$ORIGIN;$ORIGIN/../jsonrpc"
)
install(
    TARGETS mcp-client
    EXPORT snodec_mcp-client_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/mcp
            COMPONENT mcp-client
)
install(
    FILES Mcp.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/mcp/client
    COMPONENT mcp-client
)
install(
    EXPORT snodec_mcp-client_Targets
    FILE snodec_mcp-client_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mcp-client
)
