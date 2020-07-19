cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

project(snode.c)

message("Your C++ compiler supports these C++ features:")
foreach(i ${CMAKE_CXX_COMPILE_FEATURES})
  message("${i}")
endforeach()

add_subdirectory(net)
add_subdirectory(http)
add_subdirectory(log)
add_subdirectory(utils)

add_executable(snode.c main.cpp)
target_compile_options(snode.c PRIVATE --warn-all)
target_link_libraries(snode.c LINK_PUBLIC http)
install(TARGETS snode.c RUNTIME DESTINATION bin)
set_target_properties(snode.c PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

add_executable(posttest posttest.cpp)
target_compile_options(posttest PRIVATE --warn-all)
target_link_libraries(posttest LINK_PUBLIC http)
install(TARGETS posttest RUNTIME DESTINATION bin)
set_target_properties(posttest PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

add_executable(simpleserver simpleserver.cpp)
target_compile_options(simpleserver PRIVATE --warn-all)
target_link_libraries(simpleserver LINK_PUBLIC http)
install(TARGETS simpleserver RUNTIME DESTINATION bin)
set_target_properties(simpleserver PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

add_executable(experiments experiments.cpp)
target_compile_options(experiments PRIVATE --warn-all)
target_link_libraries(experiments LINK_PUBLIC http utils)
install(TARGETS experiments RUNTIME DESTINATION bin)
set_target_properties(experiments PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

if(BUILD_DOCUMENTATION)
    if (DOXYGEN_FOUND)
        set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxygen.in)
        set(DOCYFILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYFILE_IN} ${DOCYFILE} @ONLY)

        add_custom_target(BuildDocumentation
                        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_IN}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        COMMENT "Generating API documentation with Doxygen"
                        VERBATIM)

        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)
    else (DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif (DOXYGEN_FOUND)
endif(BUILD_DOCUMENTATION)

#Set the source files to clang - format
FILE(GLOB_RECURSE
    CHECK_CXX_SOURCE_FILES
    net/*.[tch]pp net/*.h
    net/ssl/*.[tch]pp net/ssl/*.h
    net/legacy/*.[tch]pp net/legacy/*.h
    http/*.[tch]pp http/*.h
    log/*.[tch]pp log/*.h
    utils/*.[tch]pp utils/*.h
)
INCLUDE("cmake/clang-cpp-checks.cmake")

find_program(iwyu_path NAMES include-what-you-use iwyu)
if(NOT iwyu_path)
  message(FATAL_ERROR "Could not find the program include-what-you-use")
endif()
option(CHECK_INCLUDES "Check used headers" ${iwyu_path}})

if(CHECK_INCLUDES)
    set_property(TARGET snode.c PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)

if(CHECK_INCLUDES)
    set_property(TARGET posttest PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)

if(CHECK_INCLUDES)
    set_property(TARGET simpleserver PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)

#target_add_ide_support(net)
#target_add_ide_support(http)
#target_add_ide_support(utils)
#target_add_ide_support(logger)

#xargs rm < install_manifest.txt
