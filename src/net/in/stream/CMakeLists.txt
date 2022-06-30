cmake_minimum_required(VERSION 3.0)

set(NET-IN-STREAM_CPP
    Socket.cpp
    config/ConfigSocketClient.cpp
    config/ConfigSocketServer.cpp
    legacy/SocketClient.cpp
    legacy/SocketServer.cpp
    tls/SocketClient.cpp
    tls/SocketServer.cpp
)

set(NET-IN-STREAM_H
    SocketClient.h
    SocketClient.hpp
    SocketServer.h
    SocketServer.hpp
    Socket.h
    config/ConfigSocketClient.h
    config/ConfigSocketServer.h
    legacy/SocketClient.h
    legacy/SocketServer.h
    legacy/config/ConfigSocketClient.h
    legacy/config/ConfigSocketServer.h
    tls/SocketClient.h
    tls/SocketServer.h
    tls/config/ConfigSocketClient.h
    tls/config/ConfigSocketServer.h
)

add_library(net-in-stream SHARED ${NET-IN-STREAM_CPP} ${NET-IN-STREAM_H})
add_library(snodec::net-in-stream ALIAS net-in-stream)

target_link_libraries(net-in-stream PUBLIC snodec::net-in)

target_include_directories(
    net-in-stream
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in-stream PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-in-stream
)

install(
    TARGETS net-in-stream
    EXPORT snodec_net-in-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in/stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-in-stream_Targets
    FILE snodec_net-in-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
