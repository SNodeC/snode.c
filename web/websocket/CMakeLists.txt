cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)

add_subdirectory(server)
add_subdirectory(subprotocol)

add_library(websocketbase SHARED Receiver.cpp Transmitter.cpp)
add_library(websocketbase::websocketbase ALIAS websocketbase)

target_link_libraries(websocketbase PUBLIC utils::utils)

target_include_directories(
    websocketbase
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/snode.c>"
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
    NAMESPACE websocketbase::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocketbase
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/websocketbaseConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocketbase
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/websocketbaseConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/websocketbase
)

if(CHECK_INCLUDES)
    set_property(
        TARGET websocketbase PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                      ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
