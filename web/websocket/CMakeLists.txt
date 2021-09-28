cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)

add_subdirectory(subprotocol)

set(WEBSOCKETBASE_INSTALL_PATH lib/snode.c/web/http/upgrade)

add_subdirectory(server)

add_library(websocketbase SHARED Receiver.cpp Transmitter.cpp)
add_library(snodec::websocketbase ALIAS websocketbase)

target_link_libraries(websocketbase PUBLIC)

target_include_directories(
    websocketbase
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

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
    EXPORT snodecwebsocketbaseTargets
    LIBRARY DESTINATION ${WEBSOCKETBASE_INSTALL_PATH}
    ARCHIVE DESTINATION ${WEBSOCKETBASE_INSTALL_PATH}
    RUNTIME DESTINATION ${WEBSOCKETBASE_INSTALL_PATH}
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
    EXPORT snodecwebsocketbaseTargets
    FILE snodecwebsocketbaseTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

if(CHECK_INCLUDES)
    set_property(
        TARGET websocketbase PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                      ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
