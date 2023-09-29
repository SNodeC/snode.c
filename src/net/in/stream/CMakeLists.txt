cmake_minimum_required(VERSION 3.18)

set(NET-IN-STREAM_CPP
    PhysicalSocket.cpp PhysicalClientSocket.cpp PhysicalServerSocket.cpp
    config/ConfigSocketClient.cpp config/ConfigSocketServer.cpp
)

set(NET-IN-STREAM_H
    PhysicalClientSocket.h
    PhysicalServerSocket.h
    SocketClient.h
    SocketServer.h
    PhysicalSocket.hpp
    PhysicalSocket.h
    config/ConfigSocketClient.h
    config/ConfigSocketServer.h
)

append_source_file_config(
    config/ConfigSocketServer.cpp REUSE_PORT "Reuse port (IPv4/IPv6)" \"false\"
    \"true\"
)

add_library(net-in-stream SHARED ${NET-IN-STREAM_CPP} ${NET-IN-STREAM_H})
add_library(snodec::net-in-stream ALIAS net-in-stream)

target_link_libraries(net-in-stream PUBLIC snodec::net-in)

target_include_directories(
    net-in-stream
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in-stream PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME
                                                           snodec-net-in-stream
)

install(
    TARGETS net-in-stream
    EXPORT snodec_net-in-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-in-stream
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in/stream
    COMPONENT net-in-stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "legacy" EXCLUDE
    PATTERN "tls" EXCLUDE
)

install(
    EXPORT snodec_net-in-stream_Targets
    FILE snodec_net-in-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-in-stream
)

add_subdirectory(legacy)
add_subdirectory(tls)
