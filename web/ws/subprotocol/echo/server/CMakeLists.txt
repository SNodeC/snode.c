cmake_minimum_required(VERSION 3.0)

add_library(echo SHARED Echo.cpp EchoInterface.cpp)

target_include_directories(echo PUBLIC ${PROJECT_SOURCE_DIR})

set_target_properties(
    echo
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

install(TARGETS echo LIBRARY DESTINATION lib/snodec/web/ws/subprotocol
                             NAMELINK_COMPONENT libecho.so.1
)
