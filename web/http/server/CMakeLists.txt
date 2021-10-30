cmake_minimum_required(VERSION 3.0)

configure_file(config.h.in config.h @ONLY)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBMAGIC libmagic)

add_library(
    httpserver SHARED
    RequestParser.cpp Request.cpp Response.cpp SocketContextUpgradeFactory.cpp
    SocketContextUpgradeFactorySelector.cpp
)
add_library(snodec::httpserver ALIAS httpserver)

if(LIBMAGIC_FOUND)
    target_compile_definitions(httpserver PUBLIC HAS_LIBMAGIC)
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
    httpserver
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${LIBMAGIC_INCLUDE_DIRS}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    httpserver PUBLIC snodec::net-ip snodec::net-tls snodec::http
                      ${LIBMAGIC_LIBRARIES} dl
)

set_target_properties(
    httpserver PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-http-server
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
    TARGETS httpserver
    EXPORT snodechttpserverTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    EXPORT snodechttpserverTargets
    FILE snodechttpserverTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
