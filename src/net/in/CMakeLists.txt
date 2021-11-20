cmake_minimum_required(VERSION 3.0)

add_library(
    net-in SHARED SocketAddress.cpp stream/Socket.cpp stream/ServerSocket.cpp
                  stream/ClientSocket.cpp
)
add_library(snodec::net-in ALIAS net-in)

target_include_directories(
    net-in
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net-in PUBLIC snodec::core-legacy snodec::core-tls)

set_target_properties(net-in PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-in)

install(
    TARGETS net-in
    EXPORT snodecnet-inTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnet-inTargets
    FILE snodecnet-inTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
