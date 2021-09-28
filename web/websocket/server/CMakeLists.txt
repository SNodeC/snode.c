cmake_minimum_required(VERSION 3.0)

add_library(
    websocketserver SHARED
    ChannelManager.cpp SocketContext.cpp SocketContextUpgradeFactory.cpp
    SubProtocol.cpp SubProtocolFactorySelector.cpp
)

add_library(snodec::websocketserver ALIAS websocketserver)

set_target_properties(
    websocketserver
    PROPERTIES
        OUTPUT_NAME "websocket"
        INSTALL_RPATH
        "${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/${WEBSOCKETBASE_INSTALL_PATH}"
)

target_link_libraries(websocketserver PUBLIC snodec::websocketbase)

target_include_directories(
    websocketserver
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    websocketserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
               # PREFIX "sc"
)

set_target_properties(
    websocketserver
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade/server"
)

install(
    TARGETS websocketserver
    EXPORT snodecwebsocketserverTargets
    LIBRARY DESTINATION lib/snode.c/web/http/upgrade/server
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    EXPORT snodecwebsocketserverTargets
    FILE snodecwebsocketserverTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

if(CHECK_INCLUDES)
    set_property(
        TARGET websocketserver PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                               ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
