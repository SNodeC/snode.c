cmake_minimum_required(VERSION 3.0)

set(CORE-LEGACY_CPP)

set(CORE-LEGACY_H
    SocketAcceptor.h
    SocketClient.h
    SocketConnection.h
    SocketConnector.h
    SocketReader.h
    SocketServer.h
    SocketWriter.h
)

add_library(core-legacy INTERFACE ${CORE-LEGACY_CPP} ${CORE-LEGACY_H})
add_library(snodec::core-legacy ALIAS core-legacy)

target_include_directories(
    core-legacy
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
              "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
              "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(core-legacy INTERFACE snodec::core)

install(
    TARGETS core-legacy
    EXPORT snodec_core-legacy_Targets
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
    EXPORT snodec_core-legacy_Targets
    FILE snodec_core-legacy_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
