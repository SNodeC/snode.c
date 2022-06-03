cmake_minimum_required(VERSION 3.0)

get_target_property(
    HTTP_SOCKETCONTEXTUPGRADE_SERVER_COMPILE_LIBDIR snodec::httpserver
    HTTP_SOCKETCONTEXTUPGRADE_SERVER_COMPILE_LIBDIR
)

get_target_property(
    HTTP_SOCKETCONTEXTUPGRADE_SERVER_INSTALL_LIBDIR snodec::httpserver
    HTTP_SOCKETCONTEXTUPGRADE_SERVER_INSTALL_LIBDIR
)

set(WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR
    "${CMAKE_BINARY_DIR}/web/websocket/server"
)
set(WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR
    "${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}/server"
)

set(WEBSOCKETSERVER_CPP
    GroupsManager.cpp SocketContextUpgrade.cpp SocketContextUpgradeFactory.cpp
    SubProtocol.cpp SubProtocolFactorySelector.cpp
)

set(WEBSOCKETSERVER_H
    GroupsManager.h SocketContextUpgrade.h SocketContextUpgradeFactory.h
    SubProtocol.h SubProtocolFactorySelector.h
)

add_library(websocketserver SHARED ${WEBSOCKETSERVER_CPP} ${WEBSOCKETSERVER_H})
add_library(snodec::websocketserver ALIAS websocketserver)

target_compile_definitions(
    websocketserver
    PRIVATE
        WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR="${WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR="${WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR}"
)

set_target_properties(
    websocketserver
    PROPERTIES
        OUTPUT_NAME "snodec-websocket"
        LIBRARY_OUTPUT_DIRECTORY
        "${HTTP_SOCKETCONTEXTUPGRADE_SERVER_COMPILE_LIBDIR}"
        INSTALL_RPATH
        "${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}"
        SOVERSION 1 # PREFIX "sc"
        WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR
        "${WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR
        "${WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_SERVER_INCLUDE_DIRS
        "${CMAKE_INSTALL_PREFIX}/include/snode.c"
        EXPORT_PROPERTIES
        "WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR;WEBSOCKET_SUBPROTOCOL_SERVER_INCLUDE_DIRS"
)

target_include_directories(
    websocketserver
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(websocketserver PUBLIC snodec::websocketbase)

install(
    TARGETS websocketserver
    EXPORT snodec_websocketserver_Targets
    LIBRARY DESTINATION ${HTTP_SOCKETCONTEXTUPGRADE_SERVER_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/websocket/server
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_websocketserver_Targets
    FILE snodec_websocketserver_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
