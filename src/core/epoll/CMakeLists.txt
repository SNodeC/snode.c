cmake_minimum_required(VERSION 3.0)

set(CORE_EPOLL_CPP DescriptorEventDispatcher.cpp EventDispatcher.cpp
                   TimerEventDispatcher.cpp
)

set(CORE_EPOLL_H DescriptorEventDispatcher.h EventDispatcher.h
                 TimerEventDispatcher.h
)

add_library(core-epoll SHARED ${CORE_EPOLL_CPP} ${CORE_EPOLL_H})
add_library(snodec::core-epoll ALIAS core-epoll)

target_include_directories(
    core-epoll
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    core-epoll PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-core-epoll
)

install(
    TARGETS core-epoll
    EXPORT snodec_core-epoll_Targets
    LIBRARY DESTINATION ${CMAKE_INISTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/core/epoll
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
)

install(
    EXPORT snodec_core-epoll_Targets
    FILE snodec_core-epoll_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
