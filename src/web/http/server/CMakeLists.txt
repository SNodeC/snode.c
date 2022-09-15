cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBMAGIC libmagic)

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
    ../legacy/in/Server.h
    ../legacy/in6/Server.h
    ../legacy/rf/Server.h
    ../legacy/un/Server.h
    ../tls/in/Server.h
    ../tls/in6/Server.h
    ../tls/rf/Server.h
    ../tls/un/Server.h
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

target_compile_definitions(
    http-server
    PRIVATE
        HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR="${HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR}"
        HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR="${CMAKE_INSTALL_PREFIX}/${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}"
)

target_include_directories(
    http-server
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${LIBMAGIC_INCLUDE_DIRS}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(http-server PUBLIC snodec::http ${LIBMAGIC_LIBRARIES} dl)

set_target_properties(
    http-server
    PROPERTIES SOVERSION 1
               OUTPUT_NAME snodec-http-server
               HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR
               "${HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR}"
               HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR
               "${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}"
               EXPORT_PROPERTIES "HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/http/server
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    TARGETS http-server
    EXPORT snodec_http-server_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/web
)

install(
    EXPORT snodec_http-server_Targets
    FILE snodec_http-server_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
