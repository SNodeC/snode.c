cmake_minimum_required(VERSION 3.18)

# snodec-control is a standalone companion project, also included by
# src/tools/CMakeLists.txt in the SNode.C tools build. It links no framework
# libraries and inspects applications only by executing separate processes.
# For an independent build, configure this directory as the CMake source root.

project(
    snodec-control
    DESCRIPTION
        "Out-of-tree companion tool to inspect SNode.C applications via -s/--show-config"
    VERSION 0.1.0
    LANGUAGES CXX
)

include(CheckPIESupported)
check_pie_supported()
option(CMAKE_POSITION_INDEPENDENT_CODE "Build position-independent code" ON)

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
