cmake_minimum_required(VERSION 3.18)

set(NET-UN-STREAM-LEGACY_CPP SocketClient.cpp SocketServer.cpp)

set(NET-UN-STREAM-LEGACY_H
    SocketClient.h SocketServer.h config/ConfigSocketClient.h
    config/ConfigSocketServer.h
)

add_library(
    net-un-stream-legacy SHARED ${NET-UN-STREAM-LEGACY_CPP}
                                ${NET-UN-STREAM-LEGACY_H}
)
add_library(snodec::net-un-stream-legacy ALIAS net-un-stream-legacy)

target_link_libraries(
    net-un-stream-legacy PUBLIC snodec::net-un-stream
                                snodec::core-socket-stream-legacy
)

target_include_directories(
    net-un-stream-legacy
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-un-stream-legacy PROPERTIES SOVERSION 1 OUTPUT_NAME
                                                snodec-net-un-stream-legacy
)

install(
    TARGETS net-un-stream-legacy
    EXPORT snodec_net-un-stream-legacy_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/un/stream/legacy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-un-stream-legacy_Targets
    FILE snodec_net-un-stream-legacy_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
