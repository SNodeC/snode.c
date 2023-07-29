cmake_minimum_required(VERSION 3.18)

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

set_source_files_properties(
    SocketContextUpgradeFactorySelector.cpp
    PROPERTIES
        COMPILE_DEFINITIONS
        "HTTP_UPGRADE_INSTALL_LIBDIR=\"${CMAKE_INSTALL_PREFIX}/${HTTP_UPGRADE_INSTALL_LIBDIR}\";SOVERSION=\"${SNODEC_SOVERSION}\""
)

target_include_directories(
    http-server
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(http-server PUBLIC snodec::http)

set_target_properties(
    http-server
    PROPERTIES
        SOVERSION ${SNODEC_SOVERSION}
        OUTPUT_NAME snodec-http-server
        LIBRARY_OUTPUT_DIRECTORY "${HTTP_COMPILE_LIBDIR}"
        BUILD_RPATH "${HTTP_COMPILE_LIBDIR}"
        HTTP_RPATH "${HTTP_RPATH}"
        HTTP_COMPILE_LIBDIR "${HTTP_COMPILE_LIBDIR}"
        HTTP_INSTALL_LIBDIR "${HTTP_INSTALL_LIBDIR}"
        HTTP_UPGRADE_COMPILE_LIBDIR "${HTTP_UPGRADE_COMPILE_LIBDIR}"
        HTTP_UPGRADE_INSTALL_LIBDIR "${HTTP_UPGRADE_INSTALL_LIBDIR}"
        EXPORT_PROPERTIES
        "HTTP_RPATH;HTTP_COMPILE_LIBDIR;HTTP_INSTALL_LIBDIR;HTTP_UPGRADE_COMPILE_LIBDIR;HTTP_UPGRADE_INSTALL_LIBDIR"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${HTTP_INSTALL_DIR}/server
    COMPONENT http-server
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    TARGETS http-server
    EXPORT snodec_http-server_Targets
    LIBRARY DESTINATION ${HTTP_INSTALL_LIBDIR} COMPONENT http-server
)

install(
    EXPORT snodec_http-server_Targets
    FILE snodec_http-server_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT http-server
)
