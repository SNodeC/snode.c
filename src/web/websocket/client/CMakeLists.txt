cmake_minimum_required(VERSION 3.0)

get_target_property(
    HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR snodec::http-client
    HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR
)

get_target_property(
    HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR snodec::http-client
    HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR
)

set(WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR
    "${CMAKE_BINARY_DIR}/web/websocket/client"
)

set(WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR
    "${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}/client"
)

set(WEBSOCKETCLIENT_CPP
    SocketContextUpgrade.cpp SocketContextUpgradeFactory.cpp SubProtocol.cpp
    SubProtocolFactorySelector.cpp
)

set(WEBSOCKETCLIENT_H SocketContextUpgrade.h SocketContextUpgradeFactory.h
                      SubProtocol.h SubProtocolFactorySelector.h
)

add_library(websocket-client SHARED ${WEBSOCKETCLIENT_CPP} ${WEBSOCKETCLIENT_H})
add_library(snodec::websocket-client ALIAS websocket-client)

target_compile_definitions(
    websocket-client
    PRIVATE
        WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR="${WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR="${WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR}"
)

set_target_properties(
    websocket-client
    PROPERTIES
        OUTPUT_NAME "snodec-websocket-client"
        LIBRARY_OUTPUT_DIRECTORY "${HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR}"
        INSTALL_RPATH
        "${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}"
        SOVERSION 1 # PREFIX "sc"
        WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR
        "${WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR
        "${WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS
        "${CMAKE_INSTALL_PREFIX}/include/snode.c"
        EXPORT_PROPERTIES
        "WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR;WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS"
)

target_include_directories(
    websocket-client
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(websocket-client PUBLIC snodec::websocket)

install(
    TARGETS websocket-client
    EXPORT snodec_websocket-client_Targets
    LIBRARY DESTINATION ${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/websocket/client
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_websocket-client_Targets
    FILE snodec_websocket-client_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
