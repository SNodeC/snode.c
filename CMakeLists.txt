# ssl-dev nlohman libmagic-dev libbluetooth-dev iwyu clang-format cmake-format

cmake_minimum_required(VERSION 3.0)

project(snode.c)

include("cmake/check-iwyu.cmake")
include("cmake/check-format.cmake")
include("cmake/check-doxygen.cmake")
include("cmake/cmake-uninstall.cmake")
include("cmake/cmake-instrumentation.cmake")
include("cmake/cmake-rpath-use-always.cmake")
include(GNUInstallDirs)

message("Your C++ compiler supports these C++ features:")
foreach(i ${CMAKE_CXX_COMPILE_FEATURES})
    message("${i}")
endforeach()

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wno-psabi
    -Wconversion
    -Wpedantic
    -pedantic-errors
    -fexec-charset=UTF-8
)

add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(net)
add_subdirectory(web)
add_subdirectory(express)
add_subdirectory(apps)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
