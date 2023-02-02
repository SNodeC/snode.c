cmake_minimum_required(VERSION 3.18)

project(snode.c)

set(ignoreMe ${CMAKE_VERBOSE_MAKEFILE})

get_filename_component(
    SNODEC_DOC_ROOTDIR "${CMAKE_SOURCE_DIR}/../snode.c-doc/" ABSOLUTE
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(format)
include(doxygen)
include(uninstall)

set(CPACK_PROJECT_NAME "snodec")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Volker Christian") # required
include(CPack)

add_custom_target(
    chown ALL
    COMMAND sudo chown -R $ENV{user}:$ENV{user} ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Make sure binary directory is writeable"
)

add_subdirectory(src)
