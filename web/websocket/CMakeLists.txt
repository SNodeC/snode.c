cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)

add_subdirectory(subprotocol)

add_library(
    websocket SHARED
    Receiver.cpp
    SocketContext.cpp
    SubProtocol.cpp
    SubProtocolFactorySelector.cpp
    Transmitter.cpp
    ws_utils.cpp
    client/SocketContext.cpp
    client/SubProtocol.cpp
    client/SubProtocolSelector.cpp
    server/SocketContext.cpp
    server/SocketContextUpgradeFactory.cpp
    server/SubProtocol.cpp
    server/SubProtocolFactory.cpp
    server/SubProtocolFactorySelector.cpp
)

target_include_directories(
    websocket PUBLIC ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(
    websocket
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
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

install(TARGETS websocket
        LIBRARY DESTINATION lib/snode.c/web/http/upgrade/server
                NAMELINK_COMPONENT libwebsocket.so.1
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/websocket
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

if(CHECK_INCLUDES)
    set_property(
        TARGET websocket PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                  ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
