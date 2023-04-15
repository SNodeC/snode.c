cmake_minimum_required(VERSION 3.18)

set(CORE_SOCKET_STREAM_CPP SocketContext.cpp SocketContextFactory.cpp
                           SocketConnection.cpp
)

set(CORE_SOCKET_STREAM_H
    SocketAcceptor.h
    SocketAcceptor.hpp
    SocketClient.h
    SocketConnection.h
    SocketConnection.hpp
    SocketConnectionFactory.h
    SocketConnectionFactory.hpp
    SocketConnector.h
    SocketConnector.hpp
    SocketContext.h
    SocketContextFactory.h
    SocketReader.h
    SocketReader.hpp
    SocketServer.h
    SocketWriter.h
)

add_library(
    core-socket-stream SHARED ${CORE_SOCKET_STREAM_CPP} ${CORE_SOCKET_STREAM_H}
)
add_library(snodec::core-socket-stream ALIAS core-socket-stream)

target_include_directories(
    core-socket-stream
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
              "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
              "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(core-socket-stream PUBLIC core-socket net-un-dgram)

set_target_properties(
    core-socket-stream PROPERTIES SOVERSION 1 OUTPUT_NAME
                                              snodec-core-socket-stream
)

install(
    TARGETS core-socket-stream
    EXPORT snodec_core-socket-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT core-socket-stream
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/socket/stream
    COMPONENT core-socket-stream
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
    COMPONENT core-socket-stream
)

add_subdirectory(legacy)
add_subdirectory(tls)
