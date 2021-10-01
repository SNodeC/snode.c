cmake_minimum_required(VERSION 3.0)

add_library(netlegacy INTERFACE)
add_library(snodec::netlegacy ALIAS netlegacy)

target_include_directories(
    netlegacy
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
              "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
              "$<INSTALL_INTERFACE:include/snode.c>"
)

install(
    TARGETS netlegacy
    EXPORT snodecnetlegacyTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/socket/stream/legacy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecnetlegacyTargets
    FILE snodecnetlegacyTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
