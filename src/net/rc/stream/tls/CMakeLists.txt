cmake_minimum_required(VERSION 3.0)

set(NET-RC-STREAM-TLS_CPP SocketClient.cpp SocketServer.cpp)

set(NET-RC-STREAM-TLS_H SocketClient.h SocketServer.h
                        config/ConfigSocketClient.h config/ConfigSocketServer.h
)

add_library(
    net-rc-stream-tls SHARED ${NET-RC-STREAM-TLS_CPP} ${NET-RC-STREAM-TLS_H}
)
add_library(snodec::net-rc-stream-tls ALIAS net-rc-stream-tls)

target_link_libraries(
    net-rc-stream-tls PUBLIC snodec::net-rc-stream
                             snodec::core-socket-stream-tls
)

target_include_directories(
    net-rc-stream-tls
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-rc-stream-tls PROPERTIES SOVERSION 1 OUTPUT_NAME
                                             snodec-net-rc-stream-tls
)

install(
    TARGETS net-rc-stream-tls
    EXPORT snodec_net-rc-stream-tls_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/rc/stream/tls
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-rc-stream-tls_Targets
    FILE snodec_net-rc-stream-tls_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
