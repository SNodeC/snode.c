cmake_minimum_required(VERSION 3.0)

add_library(echo SHARED Echo.cpp EchoInterface.cpp)

target_include_directories(echo PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(echo websocket)

set_target_properties(
    echo
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

set_target_properties(echo
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${WEBSOCKET_SERVER_LIBDIR}"
    LIBRARY_OUTPUT_DIRECTORY "${WEBSOCKET_SERVER_LIBDIR}"
    RUNTIME_OUTPUT_DIRECTORY "${WEBSOCKET_SERVER_LIBDIR}"
)

install(TARGETS echo LIBRARY DESTINATION lib/snode.c/web/websocket/subprotocol/server
                             NAMELINK_COMPONENT libecho.so.1
)
