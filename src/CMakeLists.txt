cmake_minimum_required(VERSION 3.0)

project(snode.c-code)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(iwyu)
include(instrumentation)
include(rpath-use-always)

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

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(ADDITIONAL_OPTIONS
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-documentation-unknown-command
        -Wno-exit-time-destructors
        -Wno-global-constructors
        -Wno-padded
        -Wno-reserved-identifier
        -Wno-shadow
        -Wno-shadow-field
        -Wno-used-but-marked-unused
        -Wno-weak-vtables
        -Wno-weak-template-vtables
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

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
    ${ADDITIONAL_OPTIONS}
)

# set_property(GLOBAL PROPERTY LINK_FLAGS -fsanitize=address)
# link_libraries(asan) link_libraries(ubsan)

# if (CMAKE_BUILD_TYPE MATCHES Debug) find_library(SegFault SegFault)
# message("Library: " ${SegFault}) link_libraries(${SegFault})
# add_link_options(LINKER:--no-as-needed) message ("*************** DEBUG")
# else(CMAKE_BUILD_TYPE MATCHES Debug) message ("*************** NDEBUG")
# endif(CMAKE_BUILD_TYPE MATCHES Debug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(net)
add_subdirectory(web)
add_subdirectory(express)
add_subdirectory(database)
add_subdirectory(mqtt)
add_subdirectory(apps)
