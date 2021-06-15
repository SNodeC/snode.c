cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)

add_subdirectory(subprotocol)

add_library(
    websocket SHARED
    Receiver.cpp
    SocketContext.cpp
    SubProtocol.cpp
    SubProtocolSelector.cpp
    Transmitter.cpp
    ws_utils.cpp
    client/SocketContext.cpp
    client/SubProtocol.cpp
    client/SubProtocolSelector.cpp
    server/SocketContext.cpp
    server/SocketContextUpgradeFactory.cpp
    server/SubProtocol.cpp
    server/SubProtocolSelector.cpp
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

install(TARGETS websocket LIBRARY DESTINATION lib/snode.c/web/ws
                                  NAMELINK_COMPONENT libwebsocket.so.1
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/ws
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)
