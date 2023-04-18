cmake_minimum_required(VERSION 3.18)

project(
    snode.c
    DESCRIPTION "Simple Node in C++ Frakework"
    VERSION 1.0.0
)

set(ignoreMe ${CMAKE_VERBOSE_MAKEFILE})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(format)
include(doxygen)
include(uninstall)

add_subdirectory(src)

include(Packing)
