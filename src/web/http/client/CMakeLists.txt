cmake_minimum_required(VERSION 3.18)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(DISABLED_WARNINGS -Wno-weak-template-vtables)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

set(HTTPCLIENT_CPP
    Request.cpp Response.cpp ResponseParser.cpp SocketContextUpgradeFactory.cpp
    SocketContextUpgradeFactorySelector.cpp
)

set(HTTPCLIENT_H
    Client.h
    Request.h
    Response.h
    ResponseParser.h
    SocketContext.h
    SocketContext.hpp
    SocketContextFactory.h
    SocketContextUpgradeFactory.h
    SocketContextUpgradeFactorySelector.h
)

add_library(http-client SHARED ${HTTPCLIENT_CPP} ${HTTPCLIENT_H})
add_library(snodec::http-client ALIAS http-client)

target_include_directories(
    http-client
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(http-client PUBLIC snodec::http)

get_target_property(HTTP_COMPILE_LIBDIR snodec::http HTTP_COMPILE_LIBDIR)
set(HTTP_CLIENT_COMPILE_LIBDIR "${HTTP_COMPILE_LIBDIR}/client")

get_target_property(HTTP_INSTALL_LIBDIR snodec::http HTTP_INSTALL_LIBDIR)
set(HTTP_CLIENT_INSTALL_LIBDIR "${HTTP_INSTALL_LIBDIR}/client")

get_target_property(
    HTTP_UPGRADE_COMPILE_LIBDIR snodec::http HTTP_UPGRADE_COMPILE_LIBDIR
)
set(HTTP_CLIENT_UPGRADE_COMPILE_LIBDIR "${HTTP_UPGRADE_COMPILE_LIBDIR}/client")

get_target_property(
    HTTP_UPGRADE_INSTALL_LIBDIR snodec::http HTTP_UPGRADE_INSTALL_LIBDIR
)
set(HTTP_CLIENT_UPGRADE_INSTALL_LIBDIR "${HTTP_UPGRADE_INSTALL_LIBDIR}/client")

set(HTTP_CLIENT_RPATH
    "${CMAKE_INSTALL_PREFIX}/${HTTP_INSTALL_LIBDIR};${CMAKE_INSTALL_PREFIX}/${HTTP_CLIENT_INSTALL_LIBDIR}"
)

set_target_properties(
    http-client
    PROPERTIES
        SOVERSION 1
        OUTPUT_NAME snodec-http-client
        LIBRARY_OUTPUT_DIRECTORY "${HTTP_CLIENT_COMPILE_LIBDIR}"
        COMPILE_FLAGS "${DISABLED_WARNINGS}"
        BUILD_RPATH "${HTTP_CLIENT_UPGRADE_COMPILE_LIBDIR}"
        INSTALL_RPATH
        "${CMAKE_INSTALL_PREFIX}/${HTTP_INSTALL_LIBDIR};${CMAKE_INSTALL_PREFIX}/${HTTP_CLIENT_UPGRADE_INSTALL_LIBDIR}"
        HTTP_CLIENT_COMPILE_LIBDIR "${HTTP_CLIENT_COMPILE_LIBDIR}"
        HTTP_CLIENT_INSTALL_LIBDIR "${HTTP_CLIENT_INSTALL_LIBDIR}"
        HTTP_CLIENT_UPGRADE_COMPILE_LIBDIR
        "${HTTP_CLIENT_UPGRADE_COMPILE_LIBDIR}"
        HTTP_CLIENT_UPGRADE_INSTALL_LIBDIR
        "${HTTP_CLIENT_UPGRADE_INSTALL_LIBDIR}"
        HTTP_CLIENT_RPATH "${HTTP_CLIENT_RPATH}"
        HTTP_CLIENT_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include/snode.c"
        EXPORT_PROPERTIES
        "HTTP_CLIENT_RPATH;HTTP_CLIENT_INCLUDE_DIRS;HTTP_CLIENT_UPGRADE_INSTALL_LIBDIR"
)

# target_compile_definitions(my_target PRIVATE FOO=1 BAR=1)
# set_source_files_properties(foo.cpp PROPERTIES COMPILE_DEFINITIONS -DFOO=1)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/http/client
    COMPONENT http-client
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    TARGETS http-client
    EXPORT snodec_http-client_Targets
    LIBRARY DESTINATION ${HTTP_CLIENT_INSTALL_LIBDIR} COMPONENT http-client
)

install(
    EXPORT snodec_http-client_Targets
    FILE snodec_http-client_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT http-client
)

# HTTP_CLIENT_COMPILE_LIBDIR   // Compile to HTTP_CLIENT_INSTALL_LIBDIR   //
# Install to

# HTTP_CLIENT_UPGRADE_COMPILE_LIBDIR   // RPATH to | Socket context client
# upgrade Compile to HTTP_CLIENT_UPGRADE_INSTALL_LIBDIR   // RPATH to | Socket
# context client upgrade Install to
