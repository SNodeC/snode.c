# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
find_package(PkgConfig REQUIRED)
pkg_check_modules(MCP_JSON REQUIRED nlohmann_json>=3.11)
add_library(mcp-server SHARED Server.cpp Server.h Config.cpp Config.h)
add_library(snodec::mcp-server ALIAS mcp-server)
target_compile_features(mcp-server PUBLIC cxx_std_20)
target_link_libraries(mcp-server PUBLIC core)
target_include_directories(
    mcp-server
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>" ${MCP_JSON_INCLUDE_DIRS}
)
set_target_properties(
    mcp-server
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_MCP_SOVERSION}
               OUTPUT_NAME snodec-mcp-server
)
install(
    TARGETS mcp-server
    EXPORT snodec_mcp-server_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/mcp
            COMPONENT mcp-server
)
install(
    FILES Server.h Config.h Types.h Results.h Reply.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/mcp/server
    COMPONENT mcp-server
)
install(
    EXPORT snodec_mcp-server_Targets
    FILE snodec_mcp-server_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT mcp-server
)

add_subdirectory(stdio)

add_subdirectory(http)
