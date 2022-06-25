cmake_minimum_required(VERSION 3.0)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/snode.c/web")

set(HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR ${CMAKE_CURRENT_BINARY_DIR})

set(HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR lib/snode.c/web/http)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBMAGIC libmagic)

set(HTTP_CPP MimeTypes.cpp Parser.cpp StatusCodes.cpp http_utils.cpp)

set(HTTP_H
    ConnectionState.h
    CookieOptions.h
    MimeTypes.h
    Parser.h
    SocketContext.h
    SocketContextFactory.h
    SocketContextUpgrade.h
    SocketContextUpgradeFactory.h
    SocketContextUpgradeFactory.hpp
    SocketContextUpgradeFactorySelector.h
    SocketContextUpgradeFactorySelector.hpp
)

add_library(http SHARED ${HTTP_CPP} ${HTTP_H})
add_library(snodec::http ALIAS http)

if(LIBMAGIC_FOUND)
    target_compile_definitions(http PUBLIC HAS_LIBMAGIC)
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
    http
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${LIBMAGIC_INCLUDE_DIRS}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(http PUBLIC ${LIBMAGIC_LIBRARIES} dl)

set_target_properties(
    http
    PROPERTIES SOVERSION 1
               OUTPUT_NAME snodec-http
               HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR
               "${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}"
               EXPORT_PROPERTIES "HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/http
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "server" EXCLUDE
    PATTERN "client" EXCLUDE
)

install(
    TARGETS http
    EXPORT snodec_http_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/web
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    EXPORT snodec_http_Targets
    FILE snodec_http_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

add_subdirectory(server)
add_subdirectory(client)
