cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)

set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib/snode.c/web/http/upgrade)

add_subdirectory(server)
add_subdirectory(subprotocol)

add_library(websocketbase SHARED Receiver.cpp Transmitter.cpp)
add_library(websocket::websocketbase ALIAS websocketbase)

target_link_libraries(websocketbase PUBLIC snodec::utils)

target_include_directories(
    websocketbase
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

include(GNUInstallDirs)

set_target_properties(
    websocketbase
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
               # PREFIX "sc"
)

set_target_properties(
    websocketbase
    PROPERTIES ARCHIVE_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade"
               LIBRARY_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade"
               RUNTIME_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade"
)

install(
    TARGETS websocketbase
    EXPORT websocketbaseTargets
    LIBRARY DESTINATION lib/snode.c/web/http/upgrade
            NAMELINK_COMPONENT libwebsocketbase.so.1
    ARCHIVE DESTINATION lib/snode.c/web/http/upgrade
    RUNTIME DESTINATION lib/snode.c/web/http/upgrade
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/websocket
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT websocketbaseTargets
    FILE websocketbaseTargets.cmake
    NAMESPACE websocket::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocket
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/websocketConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocket
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/websocketConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocket
)

if(CHECK_INCLUDES)
    set_property(
        TARGET websocketbase PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                      ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
