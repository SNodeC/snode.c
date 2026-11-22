# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
add_library(net-stdio SHARED SocketAddress.cpp config/ConfigAddress.cpp)
add_library(snodec::net-stdio ALIAS net-stdio)
target_link_libraries(net-stdio PUBLIC net)
target_include_directories(
    net-stdio PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                     "$<INSTALL_INTERFACE:include/snode.c>"
)
set_target_properties(
    net-stdio
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-net-stdio
)
install(
    TARGETS net-stdio
    EXPORT snodec_net-stdio_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-stdio
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/stdio
    COMPONENT net-stdio
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "phy" EXCLUDE
)
install(
    EXPORT snodec_net-stdio_Targets
    FILE snodec_net-stdio_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-stdio
)
add_subdirectory(phy)
