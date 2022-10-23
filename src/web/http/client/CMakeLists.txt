cmake_minimum_required(VERSION 3.18)

set(HTTPCLIENT_CPP
    Request.cpp Response.cpp ResponseParser.cpp SocketContextUpgradeFactory.cpp
    SocketContextUpgradeFactorySelector.cpp
)

set(HTTPCLIENT_H
    Request.h
    Response.h
    ResponseParser.h
    SocketContext.h
    SocketContext.hpp
    SocketContextFactory.h
    SocketContextUpgradeFactory.h
    SocketContextUpgradeFactorySelector.h
    ../legacy/in/Client.h
    ../legacy/in6/Client.h
    ../legacy/rc/Client.h
    ../legacy/un/Client.h
    ../tls/in/Client.h
    ../tls/in6/Client.h
    ../tls/rc/Client.h
    ../tls/un/Client.h
)

add_library(http-client SHARED ${HTTPCLIENT_CPP} ${HTTPCLIENT_H})
add_library(snodec::http-client ALIAS http-client)

if(LIBMAGIC_FOUND)
    target_compile_definitions(http-client PUBLIC HAS_LIBMAGIC)
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
    http-client
    PRIVATE
        HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR="${HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR}"
        HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR="${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}"
)

target_include_directories(
    http-client
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(http-client PUBLIC snodec::http ${LIBMAGIC_LIBRARIES} dl)

set_target_properties(
    http-client
    PROPERTIES SOVERSION 1
               OUTPUT_NAME snodec-http-client
               HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR
               "${HTTP_SOCKETCONTEXTUPGRADE_COMPILE_LIBDIR}"
               HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR
               "${HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR}"
               EXPORT_PROPERTIES "HTTP_SOCKETCONTEXTUPGRADE_INSTALL_LIBDIR"
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
    TARGETS http-client
    EXPORT snodec_http-client_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/web
)

install(
    EXPORT snodec_http-client_Targets
    FILE snodec_http-client_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
