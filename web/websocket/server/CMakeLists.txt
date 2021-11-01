cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)
configure_file(
    snodecwebsocketserverOptions.cmake.in snodecwebsocketserverOptions.cmake
    @ONLY
)

add_library(
    websocketserver SHARED
    ChannelManager.cpp SocketContextUpgrade.cpp SocketContextUpgradeFactory.cpp
    SubProtocol.cpp SubProtocolFactory.cpp SubProtocolFactorySelector.cpp
)

add_library(snodec::websocketserver ALIAS websocketserver)

set_target_properties(
    websocketserver
    PROPERTIES
        OUTPUT_NAME "snodec-websocket"
        INSTALL_RPATH
        "${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}"
        SOVERSION 1 # PREFIX "sc"
)

target_include_directories(
    websocketserver
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    websocketserver PUBLIC snodec::httpserver snodec::websocketbase
)

# set_target_properties(websocketserver PROPERTIES )

set_target_properties(
    websocketserver
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR}/server"
)

install(
    TARGETS websocketserver
    EXPORT snodecwebsocketserverTargets
    LIBRARY DESTINATION ${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}/server
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
    EXPORT snodecwebsocketserverTargets
    FILE snodecwebsocketserverTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/snodecwebsocketserverOptions.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
