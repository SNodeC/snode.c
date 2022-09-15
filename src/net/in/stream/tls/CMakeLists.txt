cmake_minimum_required(VERSION 3.0)

set(NET-IN-STREAM-TLS_CPP SocketClient.cpp SocketServer.cpp)

set(NET-IN-STREAM-TLS_H SocketClient.h SocketServer.h
                        config/ConfigSocketClient.h config/ConfigSocketServer.h
)

add_library(
    net-in-stream-tls SHARED ${NET-IN-STREAM-TLS_CPP} ${NET-IN-STREAM-TLS_H}
)
add_library(snodec::net-in-stream-tls ALIAS net-in-stream-tls)

target_link_libraries(
    net-in-stream-tls PUBLIC snodec::net-in-stream
                             snodec::core-socket-stream-tls
)

target_include_directories(
    net-in-stream-tls
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in-stream-tls PROPERTIES SOVERSION 1 OUTPUT_NAME
                                             snodec-net-in-stream-tls
)

install(
    TARGETS net-in-stream-tls
    EXPORT snodec_net-in-stream-tls_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in/stream/tls
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-in-stream-tls_Targets
    FILE snodec_net-in-stream-tls_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
