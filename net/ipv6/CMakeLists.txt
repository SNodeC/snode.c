cmake_minimum_required(VERSION 3.0)

add_library(
    net-ipv6 SHARED InetAddress.cpp stream/Socket.cpp
)
add_library(snodec::net-ipv6 ALIAS net-ipv6)

target_include_directories(
    net-ipv6
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net-ipv6 PUBLIC snodec::core-legacy snodec::core-tls)

set_target_properties(net-ipv6 PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-ipv6)

install(
    TARGETS net-ipv6
    EXPORT snodecnet-ipv6Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/ipv6
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnet-ipv6Targets
    FILE snodecnet-ipv6Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
