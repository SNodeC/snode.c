cmake_minimum_required(VERSION 3.0)

add_library(core-legacy INTERFACE)
add_library(snodec::core-legacy ALIAS core-legacy)

target_include_directories(
    core-legacy
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
              "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
              "$<INSTALL_INTERFACE:include/snode.c>"
)

install(
    TARGETS core-legacy
    EXPORT snodeccore-legacyTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/socket/stream/legacy
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodeccore-legacyTargets
    FILE snodeccore-legacyTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
