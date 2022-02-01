cmake_minimum_required(VERSION 3.0)

find_package(Backward QUIET)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(BACKWARD_FOUND)
    option(USE_BACKWARD "Compile stacktrace code")

    if(USE_BACKWARD)
        if(${STACK_DETAILS_DW})
            set(BACKWARD_LIBS "Backward::Backward")
            add_compile_definitions(BACKTRACE_SUPPORTED=1)
        endif(${STACK_DETAILS_DW})
    endif(USE_BACKWARD)
else(FACKWARD_FOUND)
    message(
        WARNING
            " backward-cpp-dev not found:\n"
            "    backward-cpp-dev is used to create stacktraces at runtime for debugging.\n"
            "    It is highly recommented to install it, if you intend to modify and debug the code of SNode.C.\n"
            "    In case you have it installed you can create stacktraces by calling stacktrace::stacktrace().\n"
            "    For that to work you have to #include \"utils/stacktrace.h\".\n"
    )
endif(BACKWARD_FOUND)

set(UTILS_CPP
    Config.cpp
    Daemon.cpp
    Timeval.cpp
    base64.cpp
    instrumentation.cpp
    sha1.cpp
    stacktrace.cpp
)

set(UTILS_H
    AttributeInjector.h
    Config.h
    Daemon.h
    Timeval.h
    base64.h
    sha1.h
    stacktrace.h
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
    EXPORT snodec_utils_Targets
    FILE snodec_utils_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
