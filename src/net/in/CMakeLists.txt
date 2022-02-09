cmake_minimum_required(VERSION 3.0)

set(NET-IN_CPP
    SocketAddress.cpp
    config/ConfigAddressExplicit.cpp
    stream/ClientSocket.cpp
    stream/ServerSocket.cpp
    stream/Socket.cpp
    stream/config/ConfigClientSocket.cpp
    stream/config/ConfigServerSocket.cpp
)

set(NET-IN_H
    SocketAddress.h
    config/ConfigAddress.h
    config/ConfigAddress.hpp
    stream/ClientSocket.h
    stream/ServerSocket.h
    stream/Socket.h
    stream/config/ConfigClientSocket.h
    stream/config/ConfigServerSocket.h
    stream/legacy/SocketClient.h
    stream/legacy/SocketServer.h
    stream/legacy/config/ConfigSocketClient.h
    stream/legacy/config/ConfigSocketServer.h
    stream/tls/SocketClient.h
    stream/tls/SocketServer.h
    stream/tls/config/ConfigSocketClient.h
    stream/tls/config/ConfigSocketServer.h
)

add_library(net-in SHARED ${NET-IN_CPP} ${NET-IN_H})
add_library(snodec::net-in ALIAS net-in)

target_link_libraries(net-in PUBLIC snodec::net)

target_include_directories(
    net-in
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(net-in PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-in)

install(
    TARGETS net-in
    EXPORT snodec_net-in_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-in_Targets
    FILE snodec_net-in_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
