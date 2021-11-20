cmake_minimum_required(VERSION 3.0)

add_library(net-ip6 SHARED SocketAddress.cpp stream/Socket.cpp)
add_library(snodec::net-ip6 ALIAS net-ip6)

target_include_directories(
    net-ip6
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net-ip6 PUBLIC snodec::core-legacy snodec::core-tls)

set_target_properties(
    net-ip6 PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-ip6
)

install(
    TARGETS net-ip6
    EXPORT snodecnet-ip6Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/ip6
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnet-ip6Targets
    FILE snodecnet-ip6Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
