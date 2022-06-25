cmake_minimum_required(VERSION 3.0)

set(NET-L2-STREAM_CPP
    Socket.cpp
    config/ConfigClientSocket.cpp
    config/ConfigServerSocket.cpp
    legacy/SocketClient.cpp
    legacy/SocketServer.cpp
    tls/SocketClient.cpp
    tls/SocketServer.cpp
)

set(NET-L2-STREAM_H
    ClientSocket.h
    ClientSocket.hpp
    ServerSocket.h
    ServerSocket.hpp
    Socket.h
    config/ConfigClientSocket.h
    config/ConfigServerSocket.h
    legacy/SocketClient.h
    legacy/SocketServer.h
    legacy/config/ConfigSocketClient.h
    legacy/config/ConfigSocketServer.h
    tls/SocketClient.h
    tls/SocketServer.h
    tls/config/ConfigSocketClient.h
    tls/config/ConfigSocketServer.h
)

add_library(net-l2-stream SHARED ${NET-L2-STREAM_CPP} ${NET-L2-STREAM_H})
add_library(snodec::net-l2-stream ALIAS net-l2-stream)

target_link_libraries(net-l2-stream PUBLIC snodec::net-l2)

target_include_directories(
    net-l2-stream
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-l2-stream PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-l2-stream
)

install(
    TARGETS net-l2-stream
    EXPORT snodec_net-l2-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/l2/stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-l2-stream_Targets
    FILE snodec_net-l2-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
