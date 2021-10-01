cmake_minimum_required(VERSION 3.0)

find_package(Backward)

if(BACKWARD_FOUND)
    option(USE_BACKWARD "Compile stacktrace code")

    if(USE_BACKWARD)
        if(${STACK_DETAILS_DW})
            set(BACKWARD_LIBS "Backward::Backward")
            target_compile_definitions(utils PUBLIC BACKTRACE_SUPPORTED=1)
        endif(${STACK_DETAILS_DW})
    endif(USE_BACKWARD)
endif(BACKWARD_FOUND)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_library(
    utils SHARED Timeval.cpp instrumentation.cpp base64.cpp stacktrace.cpp
                 sha1.cpp
)
add_library(snodec::utils ALIAS utils)

target_include_directories(
    utils
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(utils PRIVATE ${BACKWARD_LIBS} dl)

set_target_properties(utils PROPERTIES SOVERSION 1)

install(
    TARGETS utils
    EXPORT snodecutilsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/utils
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodecutilsTargets
    FILE snodecutilsTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
