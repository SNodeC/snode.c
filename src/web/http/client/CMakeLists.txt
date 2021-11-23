cmake_minimum_required(VERSION 3.0)

set(HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR
    ${HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR}/client
)

set(HTTP_SOCKETCONTEXTUPGRADE_CLIENT_INSTALL_LIBDIR
    ${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}/client
)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBMAGIC libmagic)

add_library(
    httpclient SHARED
    ResponseParser.cpp Request.cpp Response.cpp SocketContextUpgradeFactory.cpp
    SocketContextUpgradeFactorySelector.cpp
)
add_library(snodec::httpclient ALIAS httpclient)

if(LIBMAGIC_FOUND)
    target_compile_definitions(httpclient PUBLIC HAS_LIBMAGIC)
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
    httpclient
    PRIVATE
        HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR="${HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR}"
        HTTP_SOCKETCONTEXTUPGRADE_CLIENT_INSTALL_LIBDIR="${CMAKE_INSTALL_PREFIX}/${HTTP_SOCKETCONTEXTUPGRADE_CLIENT_INSTALL_LIBDIR}"
)

target_include_directories(
    httpclient
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    httpclient PUBLIC snodec::net-in snodec::net-in6 snodec::core-tls
                      snodec::http ${LIBMAGIC_LIBRARIES} dl
)

set_target_properties(
    httpclient
    PROPERTIES SOVERSION 1
               OUTPUT_NAME snodec-http-client
               HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR
               "${HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR}"
               HTTP_SOCKETCONTEXTUPGRADE_CLIENT_INSTALL_LIBDIR
               "${HTTP_SOCKETCONTEXTUPGRADE_CLIENT_INSTALL_LIBDIR}"
               EXPORT_PROPERTIES
               "HTTP_SOCKETCONTEXTUPGRADE_CLIENT_INSTALL_LIBDIR"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/http/client
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    TARGETS httpclient
    EXPORT snodec_httpclient_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    EXPORT snodec_httpclient_Targets
    FILE snodec_httpclient_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)