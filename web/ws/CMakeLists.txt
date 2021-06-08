cmake_minimum_required(VERSION 3.0)

add_subdirectory(subprotocol)

add_library(websocket SHARED
    Receiver.cpp
    SocketContext.cpp
    SubProtocol.cpp
    SubProtocolSelector.cpp
    Transmitter.cpp
    ws_utils.cpp
    client/SocketContext.cpp
    server/SocketContext.cpp
    server/SocketContextUpgradeFactory.cpp
    server/SubProtocol.cpp
    server/SubProtocolSelector.cpp)

target_include_directories(websocket PUBLIC ${CMAKE_BINARY_DIR} ${PROJECT_SOURCE_DIR})

set_target_properties(
    websocket
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

install(TARGETS websocket LIBRARY DESTINATION lib/snodec/web/ws
                             NAMELINK_COMPONENT libwebsocket.so.1
)
