cmake_minimum_required(VERSION 3.18)

set(UTILS_CPP Config.cpp Daemon.cpp Timeval.cpp base64.cpp sha1.cpp)

set(UTILS_H
    AttributeInjector.h
    CLI11.hpp
    Config.h
    Daemon.h
    Timeval.h
    base64.h
    sha1.h
)

set_source_files_properties(
    Config.cpp
    PROPERTIES
        COMPILE_FLAGS
        "-Wno-covered-switch-default -Wno-float-equal -Wno-implicit-int-float-conversion"
)

add_library(utils SHARED ${UTILS_CPP} ${UTILS_H})
add_library(snodec::utils ALIAS utils)

target_include_directories(
    utils
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(utils PRIVATE ${BACKWARD_LIBS} dl)

set_target_properties(utils PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-utils)

install(
    TARGETS utils
    EXPORT snodec_utils_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/utils
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_utils_Targets
    FILE snodec_utils_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
