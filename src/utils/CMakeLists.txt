cmake_minimum_required(VERSION 3.18)

set(UTILS_CPP
    Config.cpp
    Daemon.cpp
    PreserveErrno.cpp
    ResetToDefault.cpp
    Timeval.cpp
    Uuid.cpp
    base64.cpp
    sha1.cpp
)

set(UTILS_H
    AttributeInjector.h
    CLI11.hpp
    Config.h
    Daemon.h
    PreserveErrno.h
    ResetToDefault.h
    Timeval.h
    Uuid.h
    base64.h
    sha1.h
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

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        utils PUBLIC -Wno-undefined-func-template -Wno-covered-switch-default
    )
endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

target_compile_options(utils PUBLIC -Wno-float-equal)

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
