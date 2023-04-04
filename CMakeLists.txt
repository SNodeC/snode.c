cmake_minimum_required(VERSION 3.18)

project(
    snode.c
    DESCRIPTION "Simple Node in C++ Frakework"
    VERSION 1.0.0
)

set(ignoreMe ${CMAKE_VERBOSE_MAKEFILE})

get_filename_component(
    SNODEC_DOC_ROOTDIR "${CMAKE_SOURCE_DIR}/../snode.c-doc/" ABSOLUTE
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(format)
include(doxygen)
include(uninstall)

include(Packing)

add_subdirectory(src)
