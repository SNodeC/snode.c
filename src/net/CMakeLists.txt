cmake_minimum_required(VERSION 3.18)

add_compile_options(
    -Wno-covered-switch-default -Wno-float-equal
    -Wno-implicit-int-float-conversion -Wno-undefined-func-template
)

set(NET_CPP
    config/ConfigInstance.cpp
    config/ConfigCluster.cpp
    config/ConfigConnection.cpp
    config/ConfigLegacy.cpp
    config/ConfigListen.cpp
    config/ConfigTls.cpp
    config/ConfigTlsServer.cpp
    config/ConfigTlsClient.cpp
)

set(NET_H
    Socket.h
    Socket.hpp
    SocketAddress.h
    SocketAddress.hpp
    SocketConfig.h
    SocketConfig.hpp
    config/ConfigAddress.h
    config/ConfigAddress.hpp
    config/ConfigAddressLocal.h
    config/ConfigAddressLocal.hpp
    config/ConfigAddressRemote.h
    config/ConfigAddressRemote.hpp
    config/ConfigInstance.h
    config/ConfigCluster.h
    config/ConfigConnection.h
    config/ConfigLegacy.h
    config/ConfigListen.h
    config/ConfigTls.h
    config/ConfigTlsServer.h
    config/ConfigTlsClient.h
    dgram/PeerSocket.h
    dgram/PeerSocket.hpp
    stream/ClientSocket.h
    stream/ClientSocket.hpp
    stream/ServerSocket.h
    stream/ServerSocket.hpp
    stream/SocketClient.h
    stream/SocketClient.hpp
    stream/SocketServer.h
    stream/SocketServer.hpp
)

add_library(net SHARED ${NET_CPP} ${NET_H})
add_library(snodec::net ALIAS net)

target_include_directories(
    net
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(net PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net)

install(
    TARGETS net
    EXPORT snodec_net_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "in" EXCLUDE
    PATTERN "in6" EXCLUDE
    PATTERN "l2" EXCLUDE
    PATTERN "rc" EXCLUDE
    PATTERN "un" EXCLUDE
)

install(
    EXPORT snodec_net_Targets
    FILE snodec_net_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(in)
add_subdirectory(in6)
add_subdirectory(l2)
add_subdirectory(rc)
add_subdirectory(un)
