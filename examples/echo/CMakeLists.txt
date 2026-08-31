cmake_minimum_required(VERSION 3.18)

project(snodec-echo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(
    snodec REQUIRED
    COMPONENTS net-in-stream-legacy
)

add_library(
    echo-context STATIC
    EchoSocketContext.cpp
    EchoSocketContext.h
)

target_include_directories(
    echo-context PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    echo-context PUBLIC
    snodec::net-in-stream-legacy
)

add_executable(echoserver echoserver.cpp)
target_link_libraries(echoserver PRIVATE echo-context)

add_executable(echoclient echoclient.cpp)
target_link_libraries(echoclient PRIVATE echo-context)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
