# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
add_library(mcp-server-http SHARED Router.cpp Router.h)
add_library(snodec::mcp-server-http ALIAS mcp-server-http)
target_link_libraries(mcp-server-http PUBLIC mcp-server http-server-express)
set_target_properties(
    mcp-server-http
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-mcp-server-http
)
install(
    TARGETS mcp-server-http
    EXPORT snodec_mcp-server-http_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/mcp
            COMPONENT mcp-server-http
)
install(
    FILES Router.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/mcp/server/http
    COMPONENT mcp-server-http
)
install(
    EXPORT snodec_mcp-server-http_Targets
    FILE snodec_mcp-server-http_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mcp-server-http
)
