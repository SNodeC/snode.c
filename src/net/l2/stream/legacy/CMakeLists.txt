cmake_minimum_required(VERSION 3.18)

set(NET-L2-STREAM-LEGACY_CPP
    SocketClient.cpp SocketServer.cpp config/ConfigSocketClient.cpp
    config/ConfigSocketServer.cpp
)

set(NET-L2-STREAM-LEGACY_H
    SocketClient.h SocketServer.h config/ConfigSocketClient.h
    config/ConfigSocketServer.h
)

add_library(
    net-l2-stream-legacy SHARED ${NET-L2-STREAM-LEGACY_CPP}
                                ${NET-L2-STREAM-LEGACY_H}
)
add_library(snodec::net-l2-stream-legacy ALIAS net-l2-stream-legacy)

target_link_libraries(
    net-l2-stream-legacy PUBLIC snodec::net-l2-stream
                                snodec::core-socket-stream-legacy
)

target_include_directories(
    net-l2-stream-legacy
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-l2-stream-legacy PROPERTIES SOVERSION ${SNODEC_SO_VERSION}
                                    OUTPUT_NAME snodec-net-l2-stream-legacy
)

install(
    TARGETS net-l2-stream-legacy
    EXPORT snodec_net-l2-stream-legacy_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-l2-stream-legacy
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/l2/stream/legacy
    COMPONENT net-l2-stream-legacy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-l2-stream-legacy_Targets
    FILE snodec_net-l2-stream-legacy_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-l2-stream-legacy
)
