cmake_minimum_required(VERSION 3.0)

find_package(OpenSSL REQUIRED)

add_library(net-tls SHARED ssl_utils.cpp TLSHandshake.cpp system/ssl.cpp)
add_library(snodec::net-tls ALIAS net-tls)

target_include_directories(
    net-tls
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIRS}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(net-tls PUBLIC snodec::net ${OPENSSL_LIBRARIES})

set_target_properties(net-tls PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-tls)

install(
    TARGETS net-tls
    EXPORT snodecnet-tlsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/socket/stream/tls
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnet-tlsTargets
    FILE snodecnet-tlsTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
