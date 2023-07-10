cmake_minimum_required(VERSION 3.18)

set(NET-L2-STREAM-TLS_CPP
    SocketClient.cpp SocketServer.cpp config/ConfigSocketClient.cpp
    config/ConfigSocketServer.cpp
)

set(NET-L2-STREAM-TLS_H SocketClient.h SocketServer.h
                        config/ConfigSocketClient.h config/ConfigSocketServer.h
)

add_library(
    net-l2-stream-tls SHARED ${NET-L2-STREAM-TLS_CPP} ${NET-L2-STREAM-TLS_H}
)
add_library(snodec::net-l2-stream-tls ALIAS net-l2-stream-tls)

target_link_libraries(
    net-l2-stream-tls PUBLIC snodec::net-l2-stream
                             snodec::core-socket-stream-tls
)

target_include_directories(
    net-l2-stream-tls
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-l2-stream-tls PROPERTIES SOVERSION ${SNODEC_SOVERSION}
                                 OUTPUT_NAME snodec-net-l2-stream-tls
)

install(
    TARGETS net-l2-stream-tls
    EXPORT snodec_net-l2-stream-tls_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-l2-stream-tls
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/l2/stream/tls
    COMPONENT net-l2-stream-tls
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-l2-stream-tls_Targets
    FILE snodec_net-l2-stream-tls_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-l2-stream-tls
)
