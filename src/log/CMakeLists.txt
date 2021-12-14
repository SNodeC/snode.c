cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(EASYLOGGINGPP REQUIRED)

# ##############################################################################
# Easylogging++
# ##############################################################################
# if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/easyloggingpp/easylogging++.h)
# file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/downloads
# ${CMAKE_CURRENT_SOURCE_DIR}/easyloggingpp) message(STATUS "Start download of
# https://github.com/amrayn/easyloggingpp/archive/v9.96.7.tar.gz") file(DOWNLOAD
# https://github.com/amrayn/easyloggingpp/archive/v9.96.7.tar.gz
# ${CMAKE_CURRENT_SOURCE_DIR}/downloads/easyloggingpp-9.96.7.tar.gz STATUS
# DOWNLOAD_STATUS TIMEOUT 3) list(GET DOWNLOAD_STATUS 0 STATUS_CODE) list(GET
# DOWNLOAD_STATUS 1 ERROR_MESSAGE) if(${STATUS_CODE} EQUAL 0) message(STATUS
# "SUCESS: Download of easyloggingpp-9.96.7.tar.gz") execute_process(COMMAND tar
# --directory=${CMAKE_CURRENT_SOURCE_DIR}/downloads --extract
# --file=${CMAKE_CURRENT_SOURCE_DIR}/downloads/easyloggingpp-9.96.7.tar.gz
# RESULT_VARIABLE ret) if(${ret} EQUAL 0) message(STATUS "SUCESS: Extracting
# easyloggingpp-9.96.7.tar.gz") file(GLOB easyloggingpp_files
# ${CMAKE_CURRENT_SOURCE_DIR}/downloads/easyloggingpp-9.96.7/src/easylogging++.*)
# file(COPY ${easyloggingpp_files} DESTINATION
# ${CMAKE_CURRENT_SOURCE_DIR}/easyloggingpp) else() file(REMOVE_RECURSE
# ${CMAKE_CURRENT_SOURCE_DIR}/downloads
# ${CMAKE_CURRENT_SOURCE_DIR}/easyloggingpp) message(FATAL_ERROR "FAILURE:
# Extracting easyloggingpp-9.96.7.tar.gz") endif() else() file(REMOVE_RECURSE
# ${CMAKE_CURRENT_SOURCE_DIR}/downloads
# ${CMAKE_CURRENT_SOURCE_DIR}/easyloggingpp) message(FATAL_ERROR "Couldn't
# download\nhttps://github.com/amrayn/easyloggingpp/archive/v9.96.7.tar.gz\nno
# internet connection?") endif() endif()

set(LOGGER_CPP Logger.cpp ${EASYLOGGINGPP_INCLUDE_DIR}/easylogging++.cc)

set(LOGGER_H Logger.h)

add_library(logger SHARED ${LOGGER_CPP} ${LOGGER_H})
add_library(snodec::logger ALIAS logger)

set_source_files_properties(
    "${EASYLOGGINGPP_INCLUDE_DIR}/easylogging++.cc"
    PROPERTIES COMPILE_FLAGS
               "-Wno-unused-parameter -Wno-conversion -Wno-range-loop-construct"
)

target_compile_definitions(
    logger PRIVATE ELPP_NO_DEFAULT_LOG_FILE ELPP_CUSTOM_COUT=std::cerr
)

target_include_directories(
    logger
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
    INTERFACE ${EASYLOGGINGPP_INCLUDE_DIR}
)

set_target_properties(logger PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-logger)

install(
    TARGETS logger
    EXPORT snodec_logger_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/log
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
)
