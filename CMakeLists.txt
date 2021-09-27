# ssl-dev nlohman libmagic-dev libbluetooth-dev iwyu clang-format cmake-format

cmake_minimum_required(VERSION 3.0)

project(snode.c)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

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
    -Wpedantic
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

set(iwyu_path_and_options
    ${iwyu_path}
    -Xiwyu
    --verbose=1
    -Xiwyu
    --cxx17ns
    -Xiwyu
    --quoted_includes_first
    -Xiwyu
    --check_also='${PROJECT_SOURCE_DIR}/*'
)

add_subdirectory(log)
add_subdirectory(utils)
add_subdirectory(net)
add_subdirectory(web)
add_subdirectory(express)
add_subdirectory(apps)






configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/snodecConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)






find_package(Doxygen)
if(DOXYGEN_FOUND)
    option(
        BUILD_DOCUMENTATION
        "Create and install the HTML based API documentation (requires Doxygen)"
        ${DOXYGEN_FOUND}
    )
    if(BUILD_DOCUMENTATION)
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
    endif(BUILD_DOCUMENTATION)
else(DOXYGEN_FOUND)
    message(WARNING "Doxygen is needed to build the documentation.")
endif(DOXYGEN_FOUND)

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

# xargs rm < install_manifest.txt 140
