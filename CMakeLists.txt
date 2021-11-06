cmake_minimum_required(VERSION 3.0)

project(snode.c)

set(ignoreMe ${CMAKE_VERBOSE_MAKEFILE})

get_filename_component(
    SNODEC_DOC_ROOTDIR "${CMAKE_SOURCE_DIR}/../snode.c-doc/" ABSOLUTE
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include("cmake/check-iwyu.cmake")
include("cmake/check-format.cmake")
include("cmake/check-doxygen.cmake")
include("cmake/cmake-uninstall.cmake")
include("cmake/cmake-instrumentation.cmake")
include("cmake/cmake-rpath-use-always.cmake")

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wno-psabi # needed for RaspberryPi
    -Wconversion
    -Wpedantic
    -pedantic-errors
    -fexec-charset=UTF-8
    # -fsanitize=address -fno-omit-frame-pointer
)
# set_property(GLOBAL PROPERTY LINK_FLAGS -fsanitize=address)
# link_libraries(asan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(net)
add_subdirectory(web)
add_subdirectory(express)
add_subdirectory(apps)
