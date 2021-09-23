cmake_minimum_required(VERSION 3.0)

add_library(
    websocket SHARED
    ChannelManager.cpp SocketContext.cpp SocketContextUpgradeFactory.cpp
    SubProtocol.cpp SubProtocolFactorySelector.cpp
)

add_library(
    websocketserver::websocket ALIAS websocket)

target_link_libraries(websocket websocketbase::websocketbase utils::utils)

target_include_directories(
    websocket
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/snode.c>"
)

set_target_properties(
    websocket
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
               # PREFIX "sc"
)

set_target_properties(
    websocket
    PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade/server"
               LIBRARY_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade/server"
               RUNTIME_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade/server"
)

install(
    TARGETS websocket
    EXPORT websocketserverTargets
    LIBRARY DESTINATION lib/snode.c/web/http/upgrade/serve
            NAMELINK_COMPONENT libwebsocket.so.1
    ARCHIVE DESTINATION lib/snode.c/web/http/upgrade/serve
    RUNTIME DESTINATION lib/snode.c/web/http/upgrade/serve
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
    EXPORT websocketserverTargets
    FILE websocketserverTargets.cmake
    NAMESPACE websocketserver::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocketserver
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/websocketserverConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocketserver
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/websocketserverConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocketserver
)

if(CHECK_INCLUDES)
    set_property(
        TARGET websocket PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                  ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
