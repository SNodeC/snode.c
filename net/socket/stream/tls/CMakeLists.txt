cmake_minimum_required(VERSION 3.0)

find_package(OpenSSL REQUIRED)

add_library(nettls SHARED ssl_utils.cpp TLSHandshake.cpp system/ssl.cpp)
add_library(snodec::nettls ALIAS nettls)

target_link_libraries(nettls PUBLIC snodec::net ${OPENSSL_LIBRARIES})

set_target_properties(
    nettls
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

target_include_directories(
    nettls
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${OPENSSL_INCLUDE_DIRS}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

install(
    TARGETS nettls
    EXPORT snodecnettlsTargets
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
    EXPORT snodecnettlsTargets
    FILE snodecnettlsTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
