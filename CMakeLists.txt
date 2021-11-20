cmake_minimum_required(VERSION 3.0)

project(snode.c-master)

set(ignoreMe ${CMAKE_VERBOSE_MAKEFILE})

get_filename_component(
    SNODEC_DOC_ROOTDIR "${CMAKE_SOURCE_DIR}/../snode.c-doc/" ABSOLUTE
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(check-format)
include(check-doxygen)
include(cmake-uninstall)

add_subdirectory(src)
