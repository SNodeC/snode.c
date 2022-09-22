cmake_minimum_required(VERSION 3.0)

add_compile_options(-Wno-undefined-func-template)

set(WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR "lib/snode.c/web/websocket")

get_target_property(
    HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR snodec::http
    HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR
)

set(WEBSOCKETBASE_CPP Receiver.cpp Transmitter.cpp)
set(WEBSOCKETBASE_H
    Receiver.h
    SocketContextUpgrade.h
    SubProtocol.h
    SubProtocol.hpp
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

set_target_properties(
    websocket
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR}"
               SOVERSION 1
               OUTPUT_NAME "snodec-websocket" # PREFIX "sc"
)

install(
    TARGETS websocket
    EXPORT snodec_websocket_Targets
    LIBRARY DESTINATION ${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/websocket
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
)

add_subdirectory(server)
add_subdirectory(client)
