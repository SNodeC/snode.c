cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)

add_library(
    websocketclient SHARED
    SocketContext.cpp SocketContextUpgradeFactory.cpp
    SubProtocol.cpp
    SubProtocolFactorySelector.cpp
)

add_library(snodec::websocketserver ALIAS websocketserver)

set_target_properties(
    websocketclient
    PROPERTIES
        OUTPUT_NAME "snodec-websocket"
        INSTALL_RPATH
        "${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/${WEBSOCKETBASE_INSTALL_PATH}"
        SOVERSION 1 # PREFIX "sc"
)

target_include_directories(
    websocketclient
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(websocketclient PUBLIC snodec::http snodec::websocketbase)

# set_target_properties(websocketserver PROPERTIES )

set_target_properties(
    websocketclient
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade/client"
)

install(
    TARGETS websocketclient
    EXPORT snodecwebsocketclientTargets
    LIBRARY DESTINATION lib/snode.c/web/http/upgrade/server
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
    EXPORT snodecwebsocketclientTargets
    FILE snodecwebsocketclientTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
