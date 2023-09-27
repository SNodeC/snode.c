cmake_minimum_required(VERSION 3.18)

set(NET-IN6_CPP PhysicalSocket.cpp SocketAddress.cpp SocketAddrInfo.cpp
                config/ConfigAddress.cpp
)

set(NET-IN6_H PhysicalSocket.hpp PhysicalSocket.h SocketAddress.h
              SocketAddrInfo.h config/ConfigAddress.h
)

set(SNODEC_DEFAULT_IPV4_MAPPED $<IF:$<STREQUAL:${SNODEC_DEFAULT_IPV4_MAPPED},>,\"false\",\"true\">)
set_source_files_properties(
    config/ConfigAddress.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        SNODEC_DEFAULT_IPV4_MAPPED=${SNODEC_DEFAULT_IPV4_MAPPED}
)

add_library(net-in6 SHARED ${NET-IN6_CPP} ${NET-IN6_H})
add_library(snodec::net-in6 ALIAS net-in6)

target_link_libraries(net-in6 PUBLIC snodec::net snodec::core-socket)

target_include_directories(
    net-in6
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-in6 PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME snodec-net-in6
)

install(
    TARGETS net-in6
    EXPORT snodec_net-in6_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT net-in6
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/in6
    COMPONENT net-in6
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "stream" EXCLUDE
    PATTERN "dgram" EXCLUDE
)

install(
    EXPORT snodec_net-in6_Targets
    FILE snodec_net-in6_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT net-in6
)

add_subdirectory(stream)
