cmake_minimum_required(VERSION 3.18)

get_target_property(
    HTTP_UPGRADE_COMPILE_LIBDIR snodec::http HTTP_UPGRADE_COMPILE_LIBDIR
)
get_target_property(
    HTTP_UPGRADE_INSTALL_LIBDIR snodec::http HTTP_UPGRADE_INSTALL_LIBDIR
)

set(WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
    "${HTTP_UPGRADE_COMPILE_LIBDIR}/websocket"
)
set(WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
    "${HTTP_UPGRADE_INSTALL_LIBDIR}/websocket"
)
set(WEBSOCKET_INSTALL_INCLUDEDIR
    ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/web/websocket
)

set(WEBSOCKETBASE_CPP Receiver.cpp SubProtocolContext.cpp Transmitter.cpp)
set(WEBSOCKETBASE_H
    Receiver.h
    SocketContextUpgrade.h
    SubProtocol.h
    SubProtocol.hpp
    SubProtocolContext.h
    SubProtocolFactory.h
    SubProtocolFactorySelector.h
    Transmitter.h
)

add_library(websocket SHARED ${WEBSOCKETBASE_CPP} ${WEBSOCKETBASE_H})
add_library(snodec::websocket ALIAS websocket)

target_include_directories(
    websocket
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(websocket PUBLIC snodec::logger)

set_target_properties(
    websocket
    PROPERTIES
        OUTPUT_NAME "snodec-websocket"
        LIBRARY_OUTPUT_DIRECTORY "${HTTP_UPGRADE_COMPILE_LIBDIR}"
        SOVERSION ${SNODEC_SOVERSION}
        WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
        ${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}
        WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
        ${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}
)

install(
    TARGETS websocket
    EXPORT snodec_websocket_Targets
    LIBRARY DESTINATION ${HTTP_UPGRADE_INSTALL_LIBDIR} COMPONENT websocket
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${WEBSOCKET_INSTALL_INCLUDEDIR}
    COMPONENT websocket
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "client" EXCLUDE
    PATTERN "server" EXCLUDE
    PATTERN "subprotocol" EXCLUDE
)

install(
    EXPORT snodec_websocket_Targets
    FILE snodec_websocket_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT websocket
)

add_subdirectory(server)
add_subdirectory(client)

configure_file(
    cmake/snodec_websocket_Targets_Config.cmake.in
    snodec_websocket_Targets_Config.cmake
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/snodec_websocket_Targets_Config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT "core"
)
