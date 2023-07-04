cmake_minimum_required(VERSION 3.18)

set(HTTP_COMPILE_LIBDIR ${CMAKE_BINARY_DIR}/snode.c/http)
set(HTTP_UPGRADE_COMPILE_LIBDIR ${HTTP_COMPILE_LIBDIR}/upgrade)

set(HTTP_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/snode.c/http)
set(HTTP_UPGRADE_INSTALL_LIBDIR ${HTTP_INSTALL_LIBDIR}/upgrade)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBMAGIC libmagic)

set(HTTP_CPP MimeTypes.cpp Parser.cpp SocketContext.cpp StatusCodes.cpp
             http_utils.cpp
)

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

target_link_libraries(http PUBLIC ${LIBMAGIC_LIBRARIES})

set_target_properties(
    http
    PROPERTIES SOVERSION 1
               OUTPUT_NAME snodec-http
               LIBRARY_OUTPUT_DIRECTORY ${HTTP_COMPILE_LIBDIR}
               HTTP_COMPILE_LIBDIR ${HTTP_COMPILE_LIBDIR}
               HTTP_INSTALL_LIBDIR ${HTTP_INSTALL_LIBDIR}
               HTTP_UPGRADE_COMPILE_LIBDIR ${HTTP_UPGRADE_COMPILE_LIBDIR}
               HTTP_UPGRADE_INSTALL_LIBDIR ${HTTP_UPGRADE_INSTALL_LIBDIR}
               EXPORT_PROPERTIES HTTP_UPGRADE_INSTALL_LIBDIR
)

install(
    TARGETS http
    EXPORT snodec_http_Targets
    LIBRARY DESTINATION ${HTTP_INSTALL_LIBDIR} COMPONENT http
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/http
    COMPONENT http
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
    PATTERN "server" EXCLUDE
    PATTERN "client" EXCLUDE
)

install(
    EXPORT snodec_http_Targets
    FILE snodec_http_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT http
)

add_subdirectory(server)
add_subdirectory(client)

# HTTP_COMPILE_LIBDIR HTTP_INSTALL_LIBDIR
