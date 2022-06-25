cmake_minimum_required(VERSION 3.0)

set(CORE-SOCKET-STREAM-LEGACY_CPP)

set(CORE-SOCKET-STREAM-LEGACY_H
    SocketAcceptor.h
    SocketClient.h
    SocketConnection.h
    SocketConnector.h
    SocketReader.h
    SocketServer.h
    SocketWriter.h
)

add_library(core-socket-stream-legacy INTERFACE)
add_library(snodec::core-socket-stream-legacy ALIAS core-socket-stream-legacy)

target_include_directories(
    core-socket-stream-legacy
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
              "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
              "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(core-socket-stream-legacy INTERFACE core-socket-stream)

install(
    TARGETS core-socket-stream-legacy
    EXPORT snodec_core-socket-stream-legacy_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/socket/stream/legacy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_core-socket-stream-legacy_Targets
    FILE snodec_core-socket-stream-legacy_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
