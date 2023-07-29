cmake_minimum_required(VERSION 3.18)

get_target_property(HTTP_RPATH snodec::http-client HTTP_RPATH)

set(WEBSOCKETCLIENT_CPP
    SocketContextUpgrade.cpp SocketContextUpgradeFactory.cpp SubProtocol.cpp
    SubProtocolFactorySelector.cpp
)

set(WEBSOCKETCLIENT_H SocketContextUpgrade.h SocketContextUpgradeFactory.h
                      SubProtocol.h SubProtocolFactorySelector.h
)

add_library(websocket-client SHARED ${WEBSOCKETCLIENT_CPP} ${WEBSOCKETCLIENT_H})
add_library(snodec::websocket-client ALIAS websocket-client)

set_source_files_properties(
    SubProtocolFactorySelector.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        "WEBSOCKET_SUBPROTOCO_INSTALL_LIBDIR=\"${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}\";SOVERSION=\"${SNODEC_SOVERSION}\""
)

set_target_properties(
    websocket-client
    PROPERTIES
        OUTPUT_NAME "snodec-websocket-client"
        LIBRARY_OUTPUT_DIRECTORY "${HTTP_UPGRADE_COMPILE_LIBDIR}"
        SOVERSION ${SNODEC_SOVERSION}
        BUILD_RPATH "${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}"
        INSTALL_RPATH
        "${HTTP_RPATH};${CMAKE_INSTALL_PREFIX}/${HTTP_UPGRADE_INSTALL_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
        "${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
        "${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}"
        EXPORT_PROPERTIES
        "WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR;WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR"
)

target_include_directories(
    websocket-client
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    websocket-client PUBLIC snodec::websocket snodec::http-client
)

install(
    TARGETS websocket-client
    EXPORT snodec_websocket-client_Targets
    LIBRARY DESTINATION ${HTTP_UPGRADE_INSTALL_LIBDIR}
            COMPONENT websocket-client
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/websocket/client
    COMPONENT websocket-client
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
    COMPONENT websocket-client
)
