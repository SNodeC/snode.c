cmake_minimum_required(VERSION 3.18)

set(NET-IN6-STREAM_CPP
    PhysicalSocket.cpp PhysicalClientSocket.cpp PhysicalServerSocket.cpp
    config/ConfigSocketClient.cpp config/ConfigSocketServer.cpp
)

set(NET-IN6-STREAM_H
    PhysicalClientSocket.h
    PhysicalServerSocket.h
    SocketClient.h
    SocketServer.h
    PhysicalSocket.hpp
    PhysicalSocket.h
    config/ConfigSocketClient.h
    config/ConfigSocketServer.h
)

set(SNODEC_DEFAULT_REUSE_PORT $<IF:$<STREQUAL:${SNODEC_DEFAULT_REUSE_PORT},>,\"false\",\"true\">)
set(SNODEC_DEFAULT_IPV6_ONLY $<IF:$<STREQUAL:${SNODEC_DEFAULT_IPV6_ONLY},>,\"false\",\"true\">)
set_source_files_properties(
    config/ConfigSocketServer.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        "SNODEC_DEFAULT_REUSE_PORT=${SNODEC_DEFAULT_REUSE_PORT};\
SNODEC_DEFAULT_IPV6_ONLY=${SNODEC_DEFAULT_IPV6_ONLY}"
)

add_library(net-in6-stream SHARED ${NET-IN6-STREAM_CPP} ${NET-IN6-STREAM_H})
add_library(snodec::net-in6-stream ALIAS net-in6-stream)

target_link_libraries(net-in6-stream PUBLIC snodec::net-in6)

target_include_directories(
    net-in6-stream
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in6-stream PROPERTIES SOVERSION ${SNODEC_SOVERSION}
                              OUTPUT_NAME snodec-net-in6-stream
)

install(
    TARGETS net-in6-stream
    EXPORT snodec_net-in6-stream_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-in6-stream
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in6/stream
    COMPONENT net-in6-stream
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "legacy" EXCLUDE
    PATTERN "tls" EXCLUDE
)

install(
    EXPORT snodec_net-in6-stream_Targets
    FILE snodec_net-in6-stream_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-in6-stream
)

add_subdirectory(legacy)
add_subdirectory(tls)
