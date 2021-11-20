cmake_minimum_required(VERSION 3.0)

set(HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR ${CMAKE_CURRENT_BINARY_DIR})

set(HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR lib/snode.c/web/http)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBMAGIC libmagic)

add_library(
    http SHARED http_utils.cpp MimeTypes.cpp Parser.cpp StatusCodes.cpp
                SocketContext.cpp
)
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
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<BUILD_INTERFACE:${LIBMAGIC_INCLUDE_DIRS}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    http PUBLIC snodec::net-ip snodec::net-ip6 snodec::core-tls
                ${LIBMAGIC_LIBRARIES} dl
)

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

add_subdirectory(server)
add_subdirectory(client)
