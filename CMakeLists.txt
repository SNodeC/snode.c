cmake_minimum_required(VERSION 3.0)

project(snode.c)

add_subdirectory(net)

add_subdirectory(http)

add_executable(snode.c main.cpp)

target_link_libraries(snode.c LINK_PUBLIC http)

install(TARGETS snode.c RUNTIME DESTINATION bin)

find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

if(BUILD_DOCUMENTATION)
    if (DOXYGEN_FOUND)
        set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxygen.in)
        set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${doxyfile_in} ${doxyfile} @ONLY)

        message("Doxygen build started.")

        add_custom_target(doc
                        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile_in}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                        COMMENT "Generating API documentation with Doxygen"
                        VERBATIM)

        install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION     share/doc)
    else (DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    endif (DOXYGEN_FOUND)
endif(BUILD_DOCUMENTATION)
