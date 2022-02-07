cmake_minimum_required(VERSION 3.0)

set(NET-UN_CPP
    SocketAddress.cpp
    config/ConfigLocal.cpp
    config/ConfigRemote.cpp
    stream/ClientSocket.cpp
    stream/ServerSocket.cpp
    stream/Socket.cpp
    stream/config/ClientConfig.cpp
    stream/config/ServerConfig.cpp
)

set(NET-UN_H
    SocketAddress.h
    config/ConfigLocal.h
    config/ConfigRemote.h
    stream/ClientSocket.h
    stream/ServerSocket.h
    stream/Socket.h
    stream/config/ClientConfig.h
    stream/config/ServerConfig.h
    stream/legacy/SocketClient.h
    stream/legacy/SocketServer.h
    stream/legacy/config/ClientConfig.h
    stream/legacy/config/ServerConfig.h
    stream/tls/SocketClient.h
    stream/tls/SocketServer.h
    stream/tls/config/ClientConfig.h
    stream/tls/config/ServerConfig.h
)

add_library(net-un SHARED ${NET-UN_CPP} ${NET-UN_H})
add_library(snodec::net-un ALIAS net-un)

target_link_libraries(net-un PUBLIC snodec::net)

target_include_directories(
    net-un
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(net-un PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-un)

install(
    TARGETS net-un
    EXPORT snodec_net-un_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/un
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-un_Targets
    FILE snodec_net-un_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
