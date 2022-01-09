cmake_minimum_required(VERSION 3.0)

set(CORE_SELECT_CPP
    DescriptorEventDispatcher.cpp
    EventDispatcher.cpp
    TimerEventDispatcher.cpp)

set(CORE_SELECT_H
    DescriptorEventDispatcher.h
    EventDispatcher.h
    TimerEventDispatcher.h)

add_library(core-select SHARED ${CORE_SELECT_CPP} ${CORE_SELECT_H})
add_library(snodec::core-select ALIAS core-select)

target_include_directories(
    core-select
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(core-select PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-core-select)

install(
    TARGETS core-select
    EXPORT snodec_core-select_Targets
    LIBRARY DESTINATION ${CMAKE_INISTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/select
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp")

install(
    EXPORT snodec_core-select_Targets
    FILE snodec_core-select_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec)
