cmake_minimum_required(VERSION 3.0)

add_library(
    net-in6 SHARED SocketAddress.cpp stream/Socket.cpp stream/ServerSocket.cpp
                   stream/ClientSocket.cpp
)
add_library(snodec::net-in6 ALIAS net-in6)

target_include_directories(
    net-in6
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net-in6 PUBLIC snodec::core-legacy snodec::core-tls)

set_target_properties(net-in6 PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-in6)

install(
    TARGETS net-in6
    EXPORT snodecnet-in6Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in6
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnet-in6Targets
    FILE snodecnet-in6Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
