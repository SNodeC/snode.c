cmake_minimum_required(VERSION 3.18)

project(snode.c-code)

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

set(CMAKE_SKIP_BUILD_RPATH FALSE)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
     "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir
)
if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
endif("${isSystemDir}" STREQUAL "-1")

set(SNODEC_INTREE_BUILD "YES")

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

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT "core"
)

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
)

# https://stackoverflow.com/questions/11055569/linking-with-dynamic-library-with-dependencies
add_link_options(LINKER:--as-needed)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        # Yes, there are some unsafe-buffer-usage
        set(DISABLED_WARNINGS_CLANG -Wno-unsafe-buffer-usage)
    endif()
    add_compile_options(
        -Weverything
        # None nested namespaces is realy old fashon
        -Wno-c++98-compat-pedantic
        # Hm, with compound singletons difficult to avoid
        -Wno-exit-time-destructors
        # again, with compound singletons difficult to avoid
        -Wno-global-constructors # same, we have global compund variables
        # Yes, we do! Of course
        -Wno-shadow-field
        # Not avoidable, realy - not realy a problem nowerdays! Compiler should
        # padd
        -Wno-padded
        # Later, we want to have UML in doxygen documentation
        -Wno-documentation-unknown-command
        ${DISABLED_WARNINGS_CLANG}
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

# install(CODE "execute_process(COMMAND ldconfig)" COMPONENT notneeded)
