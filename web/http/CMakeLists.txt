cmake_minimum_required(VERSION 3.0)

set(SOCKETCONTEXTUPGRADE_SERVER_LIBDIR
    ${CMAKE_CURRENT_BINARY_DIR}
    CACHE INTERNAL "SOCKETCONTEXTUPGRADE_SERVER_LIBDIR"
)

configure_file(config.h.in config.h @ONLY)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(LibMagic REQUIRED)

add_library(
    http SHARED
    Parser.cpp
    StatusCodes.cpp
    MimeTypes.cpp
    http_utils.cpp
    client/ResponseParser.cpp
    client/Request.cpp
    client/Response.cpp
    client/SocketContextUpgradeFactorySelector.cpp
    server/RequestParser.cpp
    server/Request.cpp
    server/Response.cpp
    server/SocketContextUpgradeFactory.cpp
    server/SocketContextUpgradeFactorySelector.cpp
)
add_library(snodec::http ALIAS http)

target_include_directories(
    http
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>" "${LIBMAGIC_INCLUDES}"
)

target_link_libraries(
    http PUBLIC snodec::net-ip snodec::net-tls ${LIBMAGIC_LIBRARIES} dl
)

set_target_properties(http PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-http)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/http
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    TARGETS http
    EXPORT snodechttpTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    EXPORT snodechttpTargets
    FILE snodechttpTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
