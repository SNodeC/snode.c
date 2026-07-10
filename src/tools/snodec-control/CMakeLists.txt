cmake_minimum_required(VERSION 3.18)

# snodec-control is an out-of-tree companion tool for SNode.C applications. It
# is a fully standalone CMake project: it is never add_subdirectory()'d by the
# SNode.C build, links against none of its libraries, and inspects target
# applications purely by executing them as separate processes (never dlopen(),
# never calling their main()). Build it on its own, e.g.: mkdir build && cd
# build && cmake -DSNODEC_CONTROL_BUILD_TESTS=ON .. && cmake --build .

project(
    snodec-control
    DESCRIPTION
        "Out-of-tree companion tool to inspect SNode.C applications via -s/--show-config"
    VERSION 0.1.0
    LANGUAGES CXX
)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(
    -Wall -Wextra -Wconversion -Wuninitialized -Wunreachable-code -Wfloat-equal
)

option(SNODEC_CONTROL_BUILD_TESTS "Build snodec-control unit tests" OFF)
option(
    SNODEC_CONTROL_BUILD_TUI
    "Build the interactive curses-based configuration UI (--ui). If Curses is not found, --ui \
fails with a clear runtime message and every other feature still builds and works."
    ON
)

if(SNODEC_CONTROL_BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

add_subdirectory(src)

if(SNODEC_CONTROL_BUILD_TESTS)
    add_subdirectory(tests)
endif()
