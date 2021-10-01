cmake_minimum_required(VERSION 3.0)

add_library(
    netip SHARED address/ipv4/InetAddress.cpp address/ipv6/InetAddress.cpp
                 tcp/ipv4/Socket.cpp tcp/ipv6/Socket.cpp
)
add_library(snodec::netip ALIAS netip)

target_include_directories(
    netip
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(netip PUBLIC snodec::logger snodec::utils snodec::net)

set_target_properties(netip PROPERTIES SOVERSION 1)

install(
    TARGETS netip
    EXPORT snodecnetipTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/socket/ip
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnetipTargets
    FILE snodecnetipTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
