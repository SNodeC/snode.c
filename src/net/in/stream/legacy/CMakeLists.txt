cmake_minimum_required(VERSION 3.18)

set(NET-IN-STREAM-LEGACY_CPP
    SocketClient.cpp SocketServer.cpp config/ConfigSocketClient.cpp
    config/ConfigSocketServer.cpp
)

set(NET-IN-STREAM-LEGACY_H
    SocketClient.h SocketServer.h config/ConfigSocketClient.h
    config/ConfigSocketServer.h
)

add_library(
    net-in-stream-legacy SHARED ${NET-IN-STREAM-LEGACY_CPP}
                                ${NET-IN-STREAM-LEGACY_H}
)
add_library(snodec::net-in-stream-legacy ALIAS net-in-stream-legacy)

target_link_libraries(
    net-in-stream-legacy PUBLIC snodec::net-in-stream
                                snodec::core-socket-stream-legacy
)

target_include_directories(
    net-in-stream-legacy
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in-stream-legacy PROPERTIES SOVERSION ${SNODEC_SO_VERSION}
                                    OUTPUT_NAME snodec-net-in-stream-legacy
)

install(
    TARGETS net-in-stream-legacy
    EXPORT snodec_net-in-stream-legacy_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-in-stream-legacy
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in/stream/legacy
    COMPONENT net-in-stream-legacy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-in-stream-legacy_Targets
    FILE snodec_net-in-stream-legacy_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-in-stream-legacy
)
