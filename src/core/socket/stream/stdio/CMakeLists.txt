# SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
add_library(core-socket-stream-stdio SHARED SocketReader.cpp SocketWriter.cpp)
add_library(snodec::core-socket-stream-stdio ALIAS core-socket-stream-stdio)
target_link_libraries(core-socket-stream-stdio PUBLIC core-socket-stream)
target_include_directories(
    core-socket-stream-stdio
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
              "$<INSTALL_INTERFACE:include/snode.c>"
)
set_target_properties(
    core-socket-stream-stdio
    PROPERTIES VERSION ${SNode.C_VERSION}
               SOVERSION ${SNODEC_SOVERSION}
               OUTPUT_NAME snodec-core-socket-stream-stdio
)
install(
    TARGETS core-socket-stream-stdio
    EXPORT snodec_core-socket-stream-stdio_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT core-socket-stream-stdio
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/socket/stream/stdio
    COMPONENT core-socket-stream-stdio
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
)
install(
    EXPORT snodec_core-socket-stream-stdio_Targets
    FILE snodec_core-socket-stream-stdio_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT core-socket-stream-stdio
)
