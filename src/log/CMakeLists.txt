cmake_minimum_required(VERSION 3.18)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(EASYLOGGINGPP REQUIRED)

set(LOGGER_CPP Logger.cpp ${EASYLOGGINGPP_INCLUDE_DIR}/easylogging++.cc)

set(LOGGER_H Logger.h)

set(TMP_CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${CMAKE_CXX_INCLUDE_WHAT_YOU_USE})
# Do not use easylogging++.cc
unset(CMAKE_CXX_INCLUDE_WHAT_YOU_USE)
add_library(logger SHARED ${LOGGER_CPP} ${LOGGER_H})
add_library(snodec::logger ALIAS logger)
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${TMP_CMAKE_CXX_INCLUDE_WHAT_YOU_USE})

set(EASYLOGGING_COMPILE_FLAGS
    "\
    -Wno-conversion\
    -Wno-float-equal\
    -Wno-deprecated-copy-with-user-provided-dtor\
    -Wno-double-promotion\
    -Wno-extra-semi-stmt\
    -Wno-implicit-int-conversion\
    -Wno-implicit-int-float-conversion\
    -Wno-missing-noreturn\
    -Wno-range-loop-construct\
    -Wno-sign-conversion\
    -Wno-suggest-destructor-override\
    -Wno-suggest-override\
    -Wno-unused-exception-parameter\
    -Wno-weak-vtables\
    -Wno-documentation-unknown-command\
    "
)

set_source_files_properties(
    "${EASYLOGGINGPP_INCLUDE_DIR}/easylogging++.cc"
    PROPERTIES COMPILE_FLAGS ${EASYLOGGING_COMPILE_FLAGS}
)

target_compile_definitions(
    logger PRIVATE ELPP_NO_DEFAULT_LOG_FILE ELPP_NO_LOG_TO_FILE
                   ELPP_CUSTOM_COUT=std::cerr
)

target_include_directories(
    logger
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
    INTERFACE ${EASYLOGGINGPP_INCLUDE_DIR}
)

set_target_properties(
    logger PROPERTIES SOVERSION ${SNODEC_SOVERSION} OUTPUT_NAME snodec-logger
)

install(
    TARGETS logger
    EXPORT snodec_logger_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT logger
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/log
    COMPONENT logger
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "cmake" EXCLUDE
    PATTERN "downloads" EXCLUDE
    PATTERN "easyloggingpp" EXCLUDE
)

install(
    EXPORT snodec_logger_Targets
    FILE snodec_logger_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT logger
)
