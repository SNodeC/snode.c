cmake_minimum_required(VERSION 3.0)

add_library(
    net-ipv4 SHARED InetAddress.cpp stream/Socket.cpp
)
add_library(snodec::net-ipv4 ALIAS net-ipv4)

target_include_directories(
    net-ipv4
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net-ipv4 PUBLIC snodec::core-legacy snodec::core-tls)

set_target_properties(net-ipv4 PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-ipv4)

install(
    TARGETS net-ipv4
    EXPORT snodecnet-ipv4Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/ipv4
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnet-ipv4Targets
    FILE snodecnet-ipv4Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
