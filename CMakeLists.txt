cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

project(snode.c)

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
        set(DOXYFILE_IN ${CMAKE_SOURCE_DIR}/doc/Doxygen.in)
        set(DOXYFILE ${CMAKE_SOURCE_DIR}/doc/Doxyfile)

        configure_file(${DOXYFILE_IN} ${DOXYFILE} @ONLY)

        add_custom_target(doc
                        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_IN}
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                        COMMENT "Generating API documentation with Doxygen"
                        VERBATIM)

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/html DESTINATION share/doc)
    else (DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif (DOXYGEN_FOUND)
endif(BUILD_DOCUMENTATION)

#Set the source files to clang - format
FILE(GLOB_RECURSE
    CHECK_CXX_SOURCE_FILES
    net/*.[tch]pp net/*.h
    net/tls/*.[tch]pp net/tls/*.h
    net/legacy/*.[tch]pp net/legacy/*.h
    http/*.[tch]pp http/*.h
    http/tls/*.[tch]pp http/tls/*.h
    http/legacy/*.[tch]pp http/legacy/*.h
    log/*.[tch]pp log/*.h
    utils/*.[tch]pp utils/*.h
    apps/*.[tch]pp apps/*.h
    express/*.[tch]pp express/*.h
    express/tls/*.[tch]pp express/tls/*.h
    express/legacy/*.[tch]pp express/legacy/*.h
    express/middleware/*.[tch]pp express/middleware/*.h
)
INCLUDE("cmake/clang-cpp-checks.cmake")

#target_add_ide_support(net)
#target_add_ide_support(http)
#target_add_ide_support(utils)
#target_add_ide_support(logger)

#xargs rm < install_manifest.txt
