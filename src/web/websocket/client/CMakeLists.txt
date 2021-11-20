cmake_minimum_required(VERSION 3.0)

get_target_property(
    HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR snodec::httpclient
    HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR
)

get_target_property(
    HTTP_SOCKETCONTEXTUPGRADE_CLIENT_INSTALL_LIBDIR snodec::httpclient
    HTTP_SOCKETCONTEXTUPGRADE_CLIENT_INSTALL_LIBDIR
)

set(WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR
    "${CMAKE_BINARY_DIR}/web/websocket/client"
)

set(WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR
    "${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}/client"
)

add_library(
    websocketclient SHARED
    SocketContextUpgrade.cpp SocketContextUpgradeFactory.cpp SubProtocol.cpp
    SubProtocolFactory.cpp SubProtocolFactorySelector.cpp
)
add_library(snodec::websocketclient ALIAS websocketclient)

target_compile_definitions(
    websocketclient
    PRIVATE
        WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR="${WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR="${WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR}"
)

set_target_properties(
    websocketclient
    PROPERTIES
        OUTPUT_NAME "snodec-websocket"
        LIBRARY_OUTPUT_DIRECTORY
        "${HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR}"
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
    websocketclient
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    websocketclient PUBLIC snodec::httpclient snodec::websocketbase
)

install(
    TARGETS websocketclient
    EXPORT snodec_websocketclient_Targets
    LIBRARY DESTINATION ${HTTP_SOCKETCONTEXTUPGRADE_CLIENT_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
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
    EXPORT snodec_websocketclient_Targets
    FILE snodec_websocketclient_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
