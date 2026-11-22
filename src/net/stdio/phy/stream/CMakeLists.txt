# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
add_library(net-stdio-phy-stream SHARED PhysicalSocket.cpp PhysicalSocket.h)
add_library(snodec::net-stdio-phy-stream ALIAS net-stdio-phy-stream)
target_link_libraries(net-stdio-phy-stream PUBLIC net-stdio)
target_include_directories(
    net-stdio-phy-stream PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                                "$<INSTALL_INTERFACE:include/snode.c>"
)
set_target_properties(
    net-stdio-phy-stream
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-net-stdio-phy-stream
)
install(
    TARGETS net-stdio-phy-stream
    EXPORT snodec_net-stdio-phy-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-stdio-phy-stream
)
install(
    FILES PhysicalSocket.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/stdio/phy/stream
    COMPONENT net-stdio-phy-stream
)
install(
    EXPORT snodec_net-stdio-phy-stream_Targets
    FILE snodec_net-stdio-phy-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-stdio-phy-stream
)
