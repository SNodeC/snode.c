cmake_minimum_required(VERSION 3.0)

project(snode.c)

option(SET_RPATH "Set rpath on all binaries and libraries" ON)
if(SET_RPATH)
    set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
    set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

    # use, i.e. don't skip the full RPATH for the build tree
    set(CMAKE_SKIP_BUILD_RPATH FALSE)

    # when building, don't use the install RPATH already
    # (but later on when installing)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("SET_RPATH")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

message("Your C++ compiler supports these C++ features:")
foreach(i ${CMAKE_CXX_COMPILE_FEATURES})
  message("${i}")
endforeach()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra")

find_program(iwyu_path NAMES include-what-you-use iwyu)
if(NOT iwyu_path)
  message(FATAL_ERROR "Could not find the program include-what-you-use")
endif()

option(CHECK_INCLUDES "Check used headers")

set(iwyu_path_and_options ${iwyu_path} -Xiwyu --verbose=1)

add_subdirectory(net)
add_subdirectory(http)
add_subdirectory(express)
add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(apps)

find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

if(BUILD_DOCUMENTATION)
    if (DOXYGEN_FOUND)
        set(DOXYFILE_IN ${CMAKE_SOURCE_DIR}/docs/Doxygen.in)
        set(DOXYFILE ${CMAKE_SOURCE_DIR}/docs/Doxyfile)

        configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

        add_custom_target(doc
                        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                        COMMENT "Generating API documentation with Doxygen"
                        VERBATIM)

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/docs/html DESTINATION share/doc/snode.c)
    else (DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif (DOXYGEN_FOUND)
endif(BUILD_DOCUMENTATION)

#Set the source files to clang - format
FILE(GLOB_RECURSE
    CHECK_CXX_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/net/*.[tch]pp ${CMAKE_SOURCE_DIR}/net/*.h
    ${CMAKE_SOURCE_DIR}/http/*.[tch]pp ${CMAKE_SOURCE_DIR}/http/*.h
    ${CMAKE_SOURCE_DIR}/express/*.[tch]pp ${CMAKE_SOURCE_DIR}/express/*.h
    ${CMAKE_SOURCE_DIR}/apps/*.[tch]pp ${CMAKE_SOURCE_DIR}/apps/*.h
    ${CMAKE_SOURCE_DIR}/utils/*.[tch]pp ${CMAKE_SOURCE_DIR}/utils/*.h
    ${CMAKE_SOURCE_DIR}/log/*.[tch]pp ${CMAKE_SOURCE_DIR}/log/*.h
)
INCLUDE("cmake/clang-cpp-checks.cmake")

ADD_CUSTOM_TARGET(
    uninstall
    COMMAND
    xargs rm < install_manifest.txt
    COMMAND
    xargs -L1 dirname < install_manifest.txt | sort | uniq | xargs rmdir --ignore-fail-on-non-empty -p
    COMMENT "Uninstall project"
)

#xargs rm < install_manifest.txt
