cmake_minimum_required(VERSION 3.18)

set(CORE_SOCKET_STREAM_CPP)

set(CORE_SOCKET_STREAM_H
    SocketAcceptor.h
    SocketClient.h
    SocketConnection.h
    SocketConnectionFactory.h
    SocketConnector.h
    SocketReader.h
    SocketServer.h
    SocketWriter.h
)

add_library(core-socket-stream INTERFACE)
add_library(snodec::core-socket-stream ALIAS core-socket-stream)

target_include_directories(
    core-socket-stream
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
              "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
              "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(core-socket-stream INTERFACE core-socket net-un-dgram)

set_target_properties(
    core-socket-stream PROPERTIES SOVERSION 1 OUTPUT_NAME
                                              snodec-core-socket-stream
)

install(
    TARGETS core-socket-stream
    EXPORT snodec_core-socket-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/socket/stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "." EXCLUDE
    PATTERN "legacy" EXCLUDE
    PATTERN "tls" EXCLUDE
)

install(
    EXPORT snodec_core-socket-stream_Targets
    FILE snodec_core-socket-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(legacy)
add_subdirectory(tls)
