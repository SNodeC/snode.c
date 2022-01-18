cmake_minimum_required(VERSION 3.0)

set(CORE_POLL_CPP DescriptorEventDispatcher.cpp EventDispatcher.cpp)

set(CORE_POLL_H DescriptorEventDispatcher.h EventDispatcher.h)

add_library(core-poll SHARED ${CORE_POLL_CPP} ${CORE_POLL_H})
add_library(snodec::core-poll ALIAS core-poll)

target_include_directories(
    core-poll
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    core-poll PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-core-poll
)

install(
    TARGETS core-poll
    EXPORT snodec_core-poll_Targets
    LIBRARY DESTINATION ${CMAKE_INISTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/poll
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
)

install(
    EXPORT snodec_core-poll_Targets
    FILE snodec_core-poll_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
