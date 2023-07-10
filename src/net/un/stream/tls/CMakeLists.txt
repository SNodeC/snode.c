cmake_minimum_required(VERSION 3.18)

set(NET-UN-STREAM-TLS_CPP
    SocketClient.cpp SocketServer.cpp config/ConfigSocketClient.cpp
    config/ConfigSocketServer.cpp
)

set(NET-UN-STREAM-TLS_H SocketClient.h SocketServer.h
                        config/ConfigSocketClient.h config/ConfigSocketServer.h
)

add_library(
    net-un-stream-tls SHARED ${NET-UN-STREAM-TLS_CPP} ${NET-UN-STREAM-TLS_H}
)
add_library(snodec::net-un-stream-tls ALIAS net-un-stream-tls)

target_link_libraries(
    net-un-stream-tls PUBLIC snodec::net-un-stream
                             snodec::core-socket-stream-tls
)

target_include_directories(
    net-un-stream-tls
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-un-stream-tls PROPERTIES SOVERSION ${SNODEC_SOVERSION}
                                 OUTPUT_NAME snodec-net-un-stream-tls
)

install(
    TARGETS net-un-stream-tls
    EXPORT snodec_net-un-stream-tls_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-un-stream-tls
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/un/stream/tls
    COMPONENT net-un-stream-tls
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-un-stream-tls_Targets
    FILE snodec_net-un-stream-tls_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-un-stream-tls
)
