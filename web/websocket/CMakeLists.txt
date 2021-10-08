cmake_minimum_required(VERSION 3.0)

set(WEBSOCKETBASE_INSTALL_PATH lib/snode.c/web/http/upgrade)

add_library(websocketbase SHARED Receiver.cpp Transmitter.cpp)
add_library(snodec::websocketbase ALIAS websocketbase)

target_include_directories(
    websocketbase
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

set_target_properties(
    websocketbase
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${SOCKETCONTEXTUPGRADE_SERVER_LIBDIR}/upgrade"
               SOVERSION 1
               OUTPUT_NAME "snodec-websocketbase" # PREFIX "sc"
)

install(
    TARGETS websocketbase
    EXPORT snodecwebsocketbaseTargets
    LIBRARY DESTINATION ${WEBSOCKETBASE_INSTALL_PATH}
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
    PATTERN "server" EXCLUDE
    PATTERN "subprotocol" EXCLUDE
)

install(
    EXPORT snodecwebsocketbaseTargets
    FILE snodecwebsocketbaseTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(subprotocol)
