cmake_minimum_required(VERSION 3.18)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-weak-template-vtables)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

set(HTTPSERVER_CPP
    Request.cpp RequestContextBase.cpp RequestParser.cpp Response.cpp
    SocketContextUpgradeFactory.cpp SocketContextUpgradeFactorySelector.cpp
)

set(HTTPSERVER_H
    Request.h
    RequestContextBase.h
    RequestParser.h
    Response.h
    Server.h
    SocketContext.h
    SocketContext.hpp
    SocketContextFactory.h
    SocketContextUpgradeFactory.h
    SocketContextUpgradeFactorySelector.h
)

add_library(http-server SHARED ${HTTPSERVER_CPP} ${HTTPSERVER_h})
add_library(snodec::http-server ALIAS http-server)

if(LIBMAGIC_FOUND)
    target_compile_definitions(http-server PUBLIC HAS_LIBMAGIC)
else(LIBMAGIC_FOUND)
    message(
        WARNING
            " libmagic-dev not found:\n"
            "    libmagic is used for the automatic detection of Mime-type files.\n"
            "    SNode.C has some built-in knowledge of mime types which, compared to libmime,\n"
            "    is not as reliable. So you should install libmagic-dev for better mime type detection."
    )
endif(LIBMAGIC_FOUND)

target_include_directories(
    http-server
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(http-server PUBLIC snodec::http)

get_target_property(HTTP_COMPILE_LIBDIR snodec::http HTTP_COMPILE_LIBDIR)
set(HTTP_SERVER_COMPILE_LIBDIR "${HTTP_COMPILE_LIBDIR}/server")

get_target_property(HTTP_INSTALL_LIBDIR snodec::http HTTP_INSTALL_LIBDIR)
set(HTTP_SERVER_INSTALL_LIBDIR "${HTTP_INSTALL_LIBDIR}/server")

get_target_property(
    HTTP_UPGRADE_COMPILE_LIBDIR snodec::http HTTP_UPGRADE_COMPILE_LIBDIR
)
set(HTTP_SERVER_UPGRADE_COMPILE_LIBDIR "${HTTP_UPGRADE_COMPILE_LIBDIR}/server")

get_target_property(
    HTTP_UPGRADE_INSTALL_LIBDIR snodec::http HTTP_UPGRADE_INSTALL_LIBDIR
)
set(HTTP_SERVER_UPGRADE_INSTALL_LIBDIR "${HTTP_UPGRADE_INSTALL_LIBDIR}/server")

set(HTTP_SERVER_RPATH
    "${HTTP_RPATH};${CMAKE_INSTALL_PREFIX}/${HTTP_SERVER_INSTALL_LIBDIR}"
)

set_target_properties(
    http-server
    PROPERTIES
        SOVERSION 1
        OUTPUT_NAME snodec-http-server
        BUILD_RPATH "${HTTP_SERVER_UPGRADE_COMPILE_LIBDIR}"
        INSTALL_RPATH
        "${HTTP_RPATH};${CMAKE_INSTALL_PREFIX}/${HTTP_UPGRADE_INSTALL_LIBDIR};${CMAKE_INSTALL_PREFIX}/${HTTP_SERVER_UPGRADE_INSTALL_LIBDIR}"
        LIBRARY_OUTPUT_DIRECTORY ${HTTP_SERVER_COMPILE_LIBDIR}
        HTTP_SERVER_COMPILE_LIBDIR "${HTTP_SERVER_COMPILE_LIBDIR}"
        HTTP_SERVER_INSTALL_LIBDIR "${HTTP_SERVER_INSTALL_LIBDIR}"
        HTTP_SERVER_UPGRADE_COMPILE_LIBDIR
        "${HTTP_SERVER_UPGRADE_COMPILE_LIBDIR}"
        HTTP_SERVER_UPGRADE_INSTALL_LIBDIR
        "${HTTP_SERVER_UPGRADE_INSTALL_LIBDIR}"
        HTTP_SERVER_RPATH "${HTTP_SERVER_RPATH}"
        HTTP_SERVER_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include/snode.c"
        EXPORT_PROPERTIES
        "HTTP_SERVER_INCLUDE_DIRS;HTTP_SERVER_UPGRADE_INSTALL_LIBDIR"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/http/server
    COMPONENT http-server
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    TARGETS http-server
    EXPORT snodec_http-server_Targets
    LIBRARY DESTINATION ${HTTP_SERVER_INSTALL_LIBDIR} COMPONENT http-server
)

install(
    EXPORT snodec_http-server_Targets
    FILE snodec_http-server_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT http-server
)

# HTTP_SERVER_COMPILE_LIBDIR   // Compile to HTTP_SERVER_INSTALL_LIBDIR   //
# Install to

# HTTP_SERVER_UPGRADE_COMPILE_LIBDIR   // RPATH to | Socket context server
# upgrade Compile to HTTP_SERVER_UPGRADE_INSTALL_LIBDIR   // RPATH to | Socket
# contest server upgrade Install to
