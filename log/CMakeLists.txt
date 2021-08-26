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

add_library(
    logger SHARED Logger.cpp ${EASYLOGGINGPP_INCLUDE_DIR}/easylogging++.cc
)

set_target_properties(
    logger
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

target_compile_options(
    logger PRIVATE --warn-all -Wno-unused-parameter -Wno-conversion
                   -Wno-range-loop-construct
)

target_compile_definitions(
    logger PRIVATE ELPP_NO_DEFAULT_LOG_FILE ELPP_CUSTOM_COUT=std::cerr
)

target_include_directories(
    logger
    PUBLIC ${PROJECT_SOURCE_DIR}
    INTERFACE ${EASYLOGGINGPP_INCLUDE_DIR}
)

install(TARGETS logger LIBRARY DESTINATION lib
                               NAMELINK_COMPONENT liblogger.so.1
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

if(CHECK_INCLUDES)
    set_property(TARGET logger PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)
