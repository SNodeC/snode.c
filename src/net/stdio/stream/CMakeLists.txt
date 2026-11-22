# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
add_library(
    net-stdio-stream SHARED SocketEndpoint.cpp config/ConfigSocketEndpoint.cpp
                            SocketEndpoint.h config/ConfigSocketEndpoint.h
)
add_library(snodec::net-stdio-stream ALIAS net-stdio-stream)
target_link_libraries(
    net-stdio-stream PUBLIC net-stdio-phy-stream core-socket-stream-stdio
)
target_include_directories(
    net-stdio-stream PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                            "$<INSTALL_INTERFACE:include/snode.c>"
)
set_target_properties(
    net-stdio-stream
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-net-stdio-stream
)
install(
    TARGETS net-stdio-stream
    EXPORT snodec_net-stdio-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-stdio-stream
)
install(
    FILES SocketEndpoint.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/stdio/stream
    COMPONENT net-stdio-stream
)
install(
    FILES config/ConfigSocketEndpoint.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/stdio/stream/config
    COMPONENT net-stdio-stream
)
install(
    EXPORT snodec_net-stdio-stream_Targets
    FILE snodec_net-stdio-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-stdio-stream
)
