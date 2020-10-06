#
# CMake module for Easylogging++ logging library
#
# Defines ${EASYLOGGINGPP_INCLUDE_DIR}
#
# If ${EASYLOGGINGPP_USE_STATIC_LIBS} is ON then static libs are searched.
# In these cases ${EASYLOGGINGPP_LIBRARY} is also defined
#
# (c) 2017-2018 Zuhd Web Services
#
# https://github.com/zuhd-org/easyloggingpp
# https://zuhd.org
# https://muflihun.com
#

execute_process(COMMAND ${CMAKE_COMMAND} -E echo_append "-- Easylogging++: Searching...")
set(EASYLOGGINGPP_PATHS ${EASYLOGGINGPP_ROOT} $ENV{EASYLOGGINGPP_ROOT})

find_path(EASYLOGGINGPP_INCLUDE_DIR
        easylogging++.h
        PATH_SUFFIXES include
        PATHS ${EASYLOGGINGPP_PATHS}
)

find_package_handle_standard_args(EASYLOGGINGPP REQUIRED_VARS EASYLOGGINGPP_INCLUDE_DIR)

if (EASYLOGGINGPP_INCLUDE_DIR)
    execute_process(COMMAND ${CMAKE_COMMAND} -E echo "found")
endif()

if (EASYLOGGINGPP_USE_STATIC_LIBS)
    message ("-- Easylogging++: Static linking is preferred")
    find_library(EASYLOGGINGPP_LIBRARY
        NAMES libeasyloggingpp.a libeasyloggingpp.dylib libeasyloggingpp
        HINTS "${CMAKE_PREFIX_PATH}/lib"
    )
elseif (EASYLOGGINGPP_USE_SHARED_LIBS)
    message ("-- Easylogging++: Dynamic linking is preferred")
    find_library(EASYLOGGINGPP_LIBRARY
        NAMES libeasyloggingpp.dylib libeasyloggingpp libeasyloggingpp.a
        HINTS "${CMAKE_PREFIX_PATH}/lib"
    )
endif()
