cmake_minimum_required(VERSION 3.0)

add_library(echoserver SHARED Echo.cpp)

target_include_directories(echoserver PUBLIC ${PROJECT_SOURCE_DIR})

set_target_properties(
    echoserver
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)
