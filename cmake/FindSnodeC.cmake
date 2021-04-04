# Just copy this file into your ${CMAKE_SOURCE_DIR}/cmake directory
#
# Add set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ) to your CMakeLists.txt
# file
#
# Add find_package(SnodeC REQUIRED) to your CMakeLists.txt file
#
# In case you need snode.c without http/express add
# target_include_directories(<target> SNODEC_NET_INCLUDES)
# target_link_libraries(<target> SNODEC_NET_LIBRARIES) to your CMakeLists.txt
# file
#
# In case you need snode.c and http but not express add
# target_include_directories(<target> SNODEC_HTTP_INCLUDES)
# target_link_libraries(<target> SNODEC_HTTP_LIBRARIES) to your CMakeLists.txt
# file
#
# In case you need the full snode.c-stack (snode.c/http/express) add
# target_include_directories(<target> SNODEC_EXPRESS_INCLUDES)
# target_link_libraries(<target> SNODEC_EXPRESS_LIBRARIES) to your
# CMakeLists.txt file
#
# In case you have installed snode.c in an non-system-standard location run
# cmake with option "-DSNODEC_ROOT_DIR=<path_to_snode.c_installation> or set the
# environment variable SNODEC_ROOT_DIR to point to your
# <path_to_snode.c_installation>
#

include(
    ${CMAKE_ROOT}/${CMAKE_LIST_DIR}/Modules/FindPackageHandleStandardArgs.cmake
)

set(SNODEC_ROOT_DIR ${SNODEC_ROOT_DIR} $ENV{SNODEC_ROOT_DIR})

if(NOT SNODEC_NET_ROOT_DIR)
    set(SNODEC_NET_ROOT_DIR ${SNODEC_ROOT_DIR})
endif(NOT SNODEC_NET_ROOT_DIR)

if(NOT SNODEC_HTTP_ROOT_DIR)
    set(SNODEC_HTTP_ROOT_DIR ${SNODEC_ROOT_DIR})
endif(NOT SNODEC_HTTP_ROOT_DIR)

if(NOT SNODEC_EXPRESS_ROOT_DIR)
    set(SNODEC_EXPRESS_ROOT_DIR ${SNODEC_ROOT_DIR})
endif(NOT SNODEC_EXPRESS_ROOT_DIR)

if(NOT SNODEC_LOGGER_ROOT_DIR)
    set(SNODEC_LOGGER_ROOT_DIR ${SNODEC_ROOT_DIR})
endif(NOT SNODEC_LOGGER_ROOT_DIR)

if(NOT SNODEC_UTILS_ROOT_DIR)
    set(SNODEC_UTILS_ROOT_DIR ${SNODEC_ROOT_DIR})
endif(NOT SNODEC_UTILS_ROOT_DIR)

if(NOT SNODEC_FOUND)
    # ########## LOGGER
    find_path(
        SNODEC_LOGGER_H
        NAMES log/Logger.h
        HINTS ${SNODEC_LOGGER_ROOT_DIR}/include
        PATH_SUFFIXES snode.c
    )
    if(SNODEC_LOGGER_H)
        list(APPEND SNODEC_LOGGER_INCLUDE ${SNODEC_LOGGER_H})
    endif(SNODEC_LOGGER_H)

    find_library(
        SNODEC_LOGGER_LIBRARY logger HINTS ${SNODEC_LOGGER_ROOT_DIR}/lib
    )

    # ########## NET
    find_path(
        SNODEC_NET_H
        NAMES net/EventLoop.h
        HINTS ${SNODEC_NET_ROOT_DIR}/include
        PATH_SUFFIXES snode.c
    )
    if(SNODEC_NET_H)
        list(APPEND SNODEC_NET_INCLUDE ${SNODEC_NET_H})
    endif(SNODEC_NET_H)

    find_library(SNODEC_NET_LIBRARY net HINTS ${SNODEC_NET_ROOT_DIR}/lib)

    # ########## HTTP
    find_path(
        SNODEC_HTTP_H
        NAMES http/http_utils.h
        HINTS ${SNODEC_HTTP_ROOT_DIR}/include
        PATH_SUFFIXES snode.c
    )
    if(SNODEC_HTTP_H)
        list(APPEND SNODEC_HTTP_INCLUDE ${SNODEC_HTTP_H})
    endif(SNODEC_HTTP_H)

    find_library(SNODEC_HTTP_LIBRARY http HINTS ${SNODEC_HTTP_ROOT_DIR}/lib)

    # ########## EXPRESS
    find_path(
        SNODEC_EXPRESS_H
        NAMES express/WebApp.h
        HINTS ${SNODEC_EXPRESS_ROOT_DIR}/include
        PATH_SUFFIXES snode.c
    )
    if(SNODEC_EXPRESS_H)
        list(APPEND SNODEC_EXPRESS_INCLUDE ${SNODEC_EXPRESS_H})
    endif(SNODEC_EXPRESS_H)

    find_library(
        SNODEC_EXPRESS_LIBRARY express HINTS ${SNODEC_EXPRESS_ROOT_DIR}/lib
    )

    # ########## UTILS
    find_path(
        SNODEC_UTILS_H
        NAMES utils/AttributeInjector.h
        HINTS ${SNODEC_UTILS_ROOT_DIR}/include
        PATH_SUFFIXES snode.c
    )
    if(SNODEC_UTILS_H)
        list(APPEND SNODEC_UTILS_INCLUDE ${SNODEC_UTILS_H})
    endif(SNODEC_UTILS_H)

    find_package_handle_standard_args(
        SnodeC
        REQUIRED_VARS
            SNODEC_NET_LIBRARY
            SNODEC_NET_INCLUDE
            SNODEC_LOGGER_INCLUDE
            SNODEC_LOGGER_LIBRARY
            SNODEC_HTTP_INCLUDE
            SNODEC_HTTP_LIBRARY
            SNODEC_EXPRESS_INCLUDE
            SNODEC_EXPRESS_LIBRARY
            SNODEC_UTILS_H
        FAIL_MESSAGE
            "SnodeC not found: Define SNODEC_ROOT_DIR or set the environment \
            variable SNODEC_ROOT_DIR to point to the directory where snode.c \
            is installed."
    )
endif(NOT SNODEC_FOUND)

if(SNODEC_FOUND)
    message("-- Found SnodeC: ")
    message("--     " ${SNODEC_LOGGER_LIBRARY})
    message("--     " ${SNODEC_NET_LIBRARY})
    message("--     " ${SNODEC_HTTP_LIBRARY})
    message("--     " ${SNODEC_EXPRESS_LIBRARY})

    message("--     " ${SNODEC_LOGGER_INCLUDE})
    message("--     " ${SNODEC_NET_INCLUDE})
    message("--     " ${SNODEC_HTTP_INCLUDE})
    message("--     " ${SNODEC_EXPRESS_INCLUDE})

    set(SNODEC_NET_LIBRARIES ${SNODEC_LOGGER_LIBRARY} ${SNODEC_NET_LIBRARY})
    set(SNODEC_HTTP_LIBRARIES ${SNODEC_NET_LIBRARIES} ${SNODEC_HTTP_LIBRARY})
    set(SNODEC_EXPRESS_LIBRARIES ${SNODEC_HTTP_LIBRARIES}
                                 ${SNODEC_EXPRESS_LIBRARY}
    )

    set(SNODEC_NET_INCLUDES ${SNODEC_LOGGER_INCLUDE} ${SNODEC_NET_INCLUDE})
    set(SNODEC_HTTP_INCLUDES ${SNODEC_NET_INCLUDES} ${SNODEC_UTILS_INCLUDE}
                             ${SNODEC_HTTP_INCLUDE}
    )
    set(SNODEC_EXPRESS_INCLUDES ${SNODEC_HTTP_INCLUDES}
                                ${SNODEC_EXPRESS_INCLUDE}
    )
endif(SNODEC_FOUND)
