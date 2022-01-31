cmake_minimum_required(VERSION 3.0)

set(NET-IN6_CPP
    SocketAddress.cpp
    stream/ClientConfig.cpp
    stream/ClientSocket.cpp
    stream/ConfigBind.cpp
    stream/ConfigConnect.cpp
    stream/ServerConfig.cpp
    stream/ServerSocket.cpp
    stream/Socket.cpp
)

set(NET-IN6_H
    SocketAddress.h
    stream/ClientConfig.h
    stream/ClientSocket.h
    stream/ConfigBind.h
    stream/ConfigConnect.h
    stream/ServerConfig.h
    stream/ServerSocket.h
    stream/Socket.h
    stream/legacy/SocketClient.h
    stream/legacy/ServerConfig.h
    stream/legacy/SocketServer.h
    stream/tls/SocketClient.h
    stream/tls/ServerConfig.h
    stream/tls/SocketServer.h
)

add_library(net-in6 SHARED ${NET-IN6_CPP} ${NET-IN6_H})
add_library(snodec::net-in6 ALIAS net-in6)

target_link_libraries(net-in6 PUBLIC snodec::net)

target_include_directories(
    net-in6
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(net-in6 PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-in6)

install(
    TARGETS net-in6
    EXPORT snodec_net-in6_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in6
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-in6_Targets
    FILE snodec_net-in6_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
