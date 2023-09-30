cmake_minimum_required(VERSION 3.18)

project(
    snode.c
    DESCRIPTION "Simple Node in C++ Frakework"
    VERSION 0.90.0
)

set(ignoreMe ${CMAKE_VERBOSE_MAKEFILE})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(format)
include(doxygen)
include(uninstall)
include(graphwiz)

add_subdirectory(src)

include(Packing)
