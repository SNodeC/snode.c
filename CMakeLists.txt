# ssl-dev nlohman libmagic-dev libbluetooth-dev iwyu clang-format cmake-format

cmake_minimum_required(VERSION 3.0)

project(snode.c)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

option(SET_RPATH "Set rpath on all binaries and libraries" ON)
if(SET_RPATH)
    # use, i.e. don't skip the full RPATH for the build tree
    set(CMAKE_SKIP_BUILD_RPATH FALSE)

    # when building, don't use the install RPATH already (but later on when
    # installing)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

    # set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

    # add the automatically determined parts of the RPATH which point to
    # directories outside the build tree to the install RPATH
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

    # the RPATH to be used when installing, but only if it's not a system
    # directory
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
         "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir
    )
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif("${isSystemDir}" STREQUAL "-1")
endif(SET_RPATH)

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
    -pedantic-errors
    -fexec-charset=UTF-8
)

option(CXX_INSTRUMENT_METHODS "Instrument methods and functions")
mark_as_advanced(FORCE CXX_INSTRUMENT_METHODS)

if(CXX_INSTRUMENT_METHODS)
    add_compile_options(-finstrument-functions)
endif(CXX_INSTRUMENT_METHODS)

find_program(iwyu_path NAMES include-what-you-use iwyu)
if(NOT iwyu_path)
    message(FATAL_ERROR "Could not find the program include-what-you-use")
endif()

option(CHECK_INCLUDES "Check used headers")

set(iwyu_path_and_options ${iwyu_path} -Xiwyu --verbose=1 -Xiwyu --cxx17ns -Xiwyu --quoted_includes_first -Xiwyu --check_also='${PROJECT_SOURCE_DIR}/*')

add_subdirectory(net)
add_subdirectory(web)
add_subdirectory(express)
add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(apps)

find_package(Doxygen)
option(BUILD_DOCUMENTATION
       "Create and install the HTML based API documentation (requires Doxygen)"
       ${DOXYGEN_FOUND}
)

if(BUILD_DOCUMENTATION)
    if(DOXYGEN_FOUND)
        set(DOXYFILE_IN ${CMAKE_SOURCE_DIR}/docs/Doxygen.in)
        set(DOXYFILE ${CMAKE_SOURCE_DIR}/docs/Doxyfile)

        configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

        add_custom_target(
            doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/docs/html
                DESTINATION share/doc/snode.c
        )
    else(DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif(DOXYGEN_FOUND)
endif(BUILD_DOCUMENTATION)

add_custom_target(format DEPENDS format-cmds)
add_custom_command(
    OUTPUT format-cmds
    COMMENT "Auto formatting of all source and all cmake files"
)
set_source_files_properties(test-cmd PROPERTIES SYMBOLIC "true")

include("cmake/clang-cpp-checks.cmake")
include("cmake/cmake-format.cmake")

add_custom_target(
    uninstall
    COMMAND sudo xargs rm -v < install_manifest.txt
    COMMAND sudo xargs -L1 dirname < install_manifest.txt | sort | uniq | sudo
            xargs rmdir -v --ignore-fail-on-non-empty -p
    COMMENT "Uninstall project"
)

install(
    FILES cmake/FindSnodeC.cmake
    DESTINATION lib/cmake/SnodeC
    RENAME SnodeCConfig.cmake
)

# xargs rm < install_manifest.txt 140
