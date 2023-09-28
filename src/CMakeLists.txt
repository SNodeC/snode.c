cmake_minimum_required(VERSION 3.18)

project(snode.c-code)

set(SNODEC_INTREE_BUILD "YES")

if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12.2)
        message(
            FATAL_ERROR "GNU compiler version " ${CMAKE_CXX_COMPILER_VERSION}
                        " to old! Version required: >= 12.2"
        )
    else()
        message("Good: GNU compiler version " ${CMAKE_CXX_COMPILER_VERSION}
                " >= 12.2"
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)
        message(
            FATAL_ERROR "CLANG compiler version " ${CMAKE_CXX_COMPILER_VERSION}
                        " to old! Version required: >= 13.0"
        )
    else()
        message("Good: CLANG compiler version " ${CMAKE_CXX_COMPILER_VERSION}
                " >= 13.0"
        )
    endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(iwyu)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(AppendSourceFileConfig)

set(IO_MULTIPLEXERS "epoll;poll;select")
list(SUBLIST IO_MULTIPLEXERS 0 1 FIRST_IO_MULTIPLEXER)
set(SNODEC_DEFAULT_IO_MULTIPLEXER
    ${FIRST_IO_MULTIPLEXER}
    CACHE STRING "I/O-Multiplexer to use"
)
set_property(
    CACHE SNODEC_DEFAULT_IO_MULTIPLEXER PROPERTY STRINGS ${IO_MULTIPLEXERS}
)
message(STATUS "I/O-Multiplexer selected: '${SNODEC_DEFAULT_IO_MULTIPLEXER}'")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wconversion
    -Wpedantic
    -Wconversion
    -Wuninitialized
    -Wunreachable-code
    -Wfloat-equal
    -pedantic-errors
    -fexec-charset=UTF-8
    -Wno-shadow # yes we do
    -Wno-psabi # needed for RaspberryPi
    $<$<CXX_COMPILER_ID:dbdbClang>:-Weverything>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-c++98-compat-pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-exit-time-destructors>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-global-constructors>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-shadow-field>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-padded>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-documentation-unknown-command>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,16.0>>:-Wno-unsafe-buffer-usage>
)

# https://stackoverflow.com/questions/11055569/linking-with-dynamic-library-with-dependencies
add_link_options(LINKER:--as-needed)

set(SNODEC_SOVERSION ${snode.c_VERSION_MAJOR})

add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(net)
add_subdirectory(web)
add_subdirectory(express)
add_subdirectory(database)
add_subdirectory(iot)
add_subdirectory(apps)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/snodecConfigVersion.cmake
    VERSION "${snode.c_VERSION_MAJOR}.${snode.c_VERSION_MINOR}"
    COMPATIBILITY SameMajorVersion
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/snodecConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT "core"
)
