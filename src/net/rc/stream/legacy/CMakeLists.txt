cmake_minimum_required(VERSION 3.18)

set(NET-RC-STREAM-LEGACY_CPP SocketClient.cpp SocketServer.cpp)

set(NET-RC-STREAM-LEGACY_H
    SocketClient.h SocketServer.h config/ConfigSocketClient.h
    config/ConfigSocketServer.h
)

add_library(
    net-rc-stream-legacy SHARED ${NET-RC-STREAM-LEGACY_CPP}
                                ${NET-RC-STREAM-LEGACY_H}
)
add_library(snodec::net-rc-stream-legacy ALIAS net-rc-stream-legacy)

target_link_libraries(
    net-rc-stream-legacy PUBLIC snodec::net-rc-stream
                                snodec::core-socket-stream-legacy
)

target_include_directories(
    net-rc-stream-legacy
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-rc-stream-legacy PROPERTIES SOVERSION 1 OUTPUT_NAME
                                                snodec-net-rc-stream-legacy
)

install(
    TARGETS net-rc-stream-legacy
    EXPORT snodec_net-rc-stream-legacy_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/rc/stream/legacy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-rc-stream-legacy_Targets
    FILE snodec_net-rc-stream-legacy_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
