cmake_minimum_required(VERSION 3.0)

add_library(net-ip SHARED SocketAddress.cpp stream/Socket.cpp)
add_library(snodec::net-ip ALIAS net-ip)

target_include_directories(
    net-ip
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net-ip PUBLIC snodec::core-legacy snodec::core-tls)

set_target_properties(
    net-ip PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-ip
)

install(
    TARGETS net-ip
    EXPORT snodecnet-ipTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/ip
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnet-ipTargets
    FILE snodecnet-ipTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
