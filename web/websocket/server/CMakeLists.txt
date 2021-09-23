cmake_minimum_required(VERSION 3.0)

add_library(
    server SHARED
    ChannelManager.cpp SocketContext.cpp SocketContextUpgradeFactory.cpp
    SubProtocol.cpp SubProtocolFactorySelector.cpp
)

set_target_properties(server PROPERTIES OUTPUT_NAME "websocket")
set_target_properties(
    server PROPERTIES INSTALL_RPATH
                      ${CMAKE_INSTALL_PREFIX}/lib/snode.c/web/http/upgrade
)

add_library(websocket::server ALIAS server)

target_link_libraries(server PUBLIC websocket::websocketbase utils::utils)

target_include_directories(
    server
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    server
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
               # PREFIX "sc"
)

set_target_properties(
    server
    PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade/server"
               LIBRARY_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade/server"
               RUNTIME_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade/server"
)

install(
    TARGETS server
    EXPORT websocketserverTargets
    LIBRARY DESTINATION lib/snode.c/web/http/upgrade/server
            NAMELINK_COMPONENT websocket
    ARCHIVE DESTINATION lib/snode.c/web/http/upgrade/server
    RUNTIME DESTINATION lib/snode.c/web/http/upgrade/server
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
    NAMESPACE websocket::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocket
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/websocketserverConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocket
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/websocketserverConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocket
)

if(CHECK_INCLUDES)
    set_property(
        TARGET websocket PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                  ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
