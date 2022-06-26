cmake_minimum_required(VERSION 3.0)

set(NET-UN-DGRAM_CPP
    Socket.cpp
)

set(NET-UN-DGRAM_H
    Socket.h
)

add_library(net-un-dgram SHARED ${NET-UN-DGRAM_CPP} ${NET-UN-DGRAM_H})
add_library(snodec::net-un-dgram ALIAS net-un-dgram)

target_link_libraries(net-un-dgram PUBLIC snodec::net-un)

target_include_directories(
    net-un-dgram
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    net-un-dgram PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-net-un-dgram
)

install(
    TARGETS net-un-dgram
    EXPORT snodec_net-un-dgram_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/net/un/dgram
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_net-un-dgram_Targets
    FILE snodec_net-un-dgram_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
