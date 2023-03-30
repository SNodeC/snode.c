cmake_minimum_required(VERSION 3.18)

set(NET-RC-STREAM_CPP
    PhysicalClientSocket.cpp PhysicalServerSocket.cpp
    config/ConfigSocketClient.cpp config/ConfigSocketServer.cpp
)

set(NET-RC-STREAM_H
    PhysicalClientSocket.h
    PhysicalServerSocket.h
    SocketClient.h
    SocketServer.h
    PhysicalSocket.hpp
    PhysicalSocket.h
    config/ConfigSocketClient.h
    config/ConfigSocketServer.h
)

add_library(net-rc-stream SHARED ${NET-RC-STREAM_CPP} ${NET-RC-STREAM_H})
add_library(snodec::net-rc-stream ALIAS net-rc-stream)

target_link_libraries(net-rc-stream PUBLIC snodec::net-rc)

target_include_directories(
    net-rc-stream
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-rc-stream PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-rc-stream
)

install(
    TARGETS net-rc-stream
    EXPORT snodec_net-rc-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/rc/stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "legacy" EXCLUDE
    PATTERN "tls" EXCLUDE
)

install(
    EXPORT snodec_net-rc-stream_Targets
    FILE snodec_net-rc-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(legacy)
add_subdirectory(tls)
