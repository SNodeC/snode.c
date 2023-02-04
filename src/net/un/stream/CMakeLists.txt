cmake_minimum_required(VERSION 3.18)

set(NET-UN-STREAM_CPP
    PhysicalClientSocket.cpp PhysicalServerSocket.cpp PhysicalSocket.cpp config/ConfigSocketClient.cpp
    config/ConfigSocketServer.cpp
)

set(NET-UN-STREAM_H
    PhysicalClientSocket.h
    PhysicalServerSocket.h
    SocketClient.h
    SocketClient.hpp
    SocketServer.h
    SocketServer.hpp
    PhysicalSocket.h
    config/ConfigSocketClient.h
    config/ConfigSocketServer.h
)

add_library(net-un-stream SHARED ${NET-UN-STREAM_CPP} ${NET-UN-STREAM_H})
add_library(snodec::net-un-stream ALIAS net-un-stream)

target_link_libraries(net-un-stream PUBLIC snodec::net-un)

target_include_directories(
    net-un-stream
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-un-stream PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-un-stream
)

install(
    TARGETS net-un-stream
    EXPORT snodec_net-un-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/un/stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "legacy" EXCLUDE
    PATTERN "tls" EXCLUDE
)

install(
    EXPORT snodec_net-un-stream_Targets
    FILE snodec_net-un-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(legacy)
add_subdirectory(tls)
