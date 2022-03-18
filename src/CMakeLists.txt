cmake_minimum_required(VERSION 3.0)

project(snode.c-code)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(iwyu)
include(instrumentation)
include(rpath-use-always)

set(IO_Multiplexer
    "epoll"
    CACHE STRING "I/O-Multiplexer to use"
)
set(IO_Multiplexers "epoll;poll;select")
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

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wno-psabi # needed for RaspberryPi
    -Wconversion
    -Wpedantic
    -pedantic-errors
    -fexec-charset=UTF-8
    # -Weverything -Wno-c++17-compat -Wno-c++98-compat
    # -Wno-c++98-compat-pedantic -Wno-reserved-identifier
    # -Wno-shadow-field-in-constructor -Wno-padded -Wno-shadow
    # -Wno-global-constructors -Wno-shadow-field -Wno-weak-vtables
    # -Wno-weak-template-vtables -Wno-exit-time-destructors
    # -Wno-missing-prototypes -Wno-undefined-func-template # has to be
    # investigated why -Wno-documentation-unknown-command -Wno-double-promotion
    # -Wno-covered-switch-default -Wno-used-but-marked-unused -fsanitize=address
    # -fno-omit-frame-pointer
)
# set_property(GLOBAL PROPERTY LINK_FLAGS -fsanitize=address)
# link_libraries(asan) link_libraries(ubsan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(net)
add_subdirectory(web)
add_subdirectory(express)
add_subdirectory(apps)
add_subdirectory(database)
