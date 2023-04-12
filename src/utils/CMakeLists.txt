cmake_minimum_required(VERSION 3.18)

set(UTILS_CPP
    Config.cpp
    Daemon.cpp
    Formatter.cpp
    PreserveErrno.cpp
    ResetToDefault.cpp
    Timeval.cpp
    Uuid.cpp
    base64.cpp
    sha1.cpp
    system/signal.cpp
    system/time.cpp
)

set(UTILS_H
    AttributeInjector.h
    CLI11.hpp
    Config.h
    Daemon.h
    Formatter.h
    PreserveErrno.h
    ResetToDefault.h
    Timeval.h
    Uuid.h
    Validators.hpp
    base64.h
    sha1.h
    system/signal.h
    system/time.h
)

add_library(utils SHARED ${UTILS_CPP} ${UTILS_H})
add_library(snodec::utils ALIAS utils)

target_include_directories(
    utils
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(utils PUBLIC snodec::logger)

set_target_properties(utils PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-utils)

install(
    TARGETS utils
    EXPORT snodec_utils_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT utils
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/utils
    COMPONENT utils
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_utils_Targets
    FILE snodec_utils_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT utils
)
