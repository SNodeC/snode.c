cmake_minimum_required(VERSION 3.18)

project(snode.c-code)

set(SNODEC_INTREE_BUILD "YES")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(iwyu)

set(IO_Multiplexers "epoll;poll;select")
list(SUBLIST IO_Multiplexers 0 1 IO_DefaultMultiplexer)
set(IO_Multiplexer
    ${IO_DefaultMultiplexer}
    CACHE STRING "I/O-Multiplexer to use"
)
set_property(CACHE IO_Multiplexer PROPERTY STRINGS ${IO_Multiplexers})
message(STATUS "I/O-Multiplexer selected: '${IO_Multiplexer}'")

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wno-psabi # needed for RaspberryPi
    -Wconversion
    -Wpedantic
    -Wconversion
    -Wuninitialized
    -Wunreachable-code
    -Wfloat-equal
    -pedantic-errors
    -fexec-charset=UTF-8
    -Wno-shadow
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-shadow-field
        -Wno-shadow-field-in-constructor
        -Wno-padded
        -Wno-reserved-identifier
        -Wno-documentation-unknown-command
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(net)
add_subdirectory(web)
add_subdirectory(express)
add_subdirectory(database)
add_subdirectory(iot)
add_subdirectory(apps)

install(CODE "execute_process(COMMAND ldconfig)")
