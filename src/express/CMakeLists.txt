# snode.c - a slim toolkit for network communication Copyright (C) 2020, 2021,
# 2022, 2023 Volker Christian <me@vchrist.at>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.18)

find_package(PkgConfig REQUIRED)

pkg_check_modules(NLOHMANN_JSON nlohmann_json>=3.11)

if(NLOHMANN_JSON_FOUND)
    set(JSONMIDDLEWARE_CPP middleware/JsonMiddleware.cpp)
    set(JSONMIDDLEWARE_H middleware/JsonMiddleware.h)
else(NLOHMANN_JSON_FOUND)
    message(
        FATAL_ERROR
            " nlohmann-json3-dev not found:\n"
            "   nlohmann_json is used for http. Plaase install it by executing\n"
            "       sudo apt install nlohmann-json3-dev"
    )
endif(NLOHMANN_JSON_FOUND)

set(HTTP_SERVER_EXPRESS_CPP
    Controller.cpp
    Dispatcher.cpp
    MountPoint.cpp
    Next.cpp
    Request.cpp
    Response.cpp
    Route.cpp
    RootRoute.cpp
    Router.cpp
    WebApp.cpp
    dispatcher/ApplicationDispatcher.cpp
    dispatcher/MiddlewareDispatcher.cpp
    dispatcher/RouterDispatcher.cpp
    dispatcher/regex_utils.cpp
    middleware/BasicAuthentication.cpp
    middleware/StaticMiddleware.cpp
    middleware/VHost.cpp
    ${JSONMIDDLEWARE_CPP}
)

set(HTTP_SERVER_EXPRESS_H
    Controller.h
    Dispatcher.h
    MountPoint.h
    Next.h
    Request.h
    Response.h
    Route.h
    Route.hpp
    Router.h
    RootRoute.h
    WebApp.h
    WebAppT.h
    dispatcher/ApplicationDispatcher.h
    dispatcher/MiddlewareDispatcher.h
    dispatcher/RouterDispatcher.h
    dispatcher/regex_utils.h
    middleware/BasicAuthentication.h
    middleware/StaticMiddleware.h
    middleware/VHost.h
    legacy/in/WebApp.h
    legacy/in6/WebApp.h
    legacy/rc/WebApp.h
    tls/in/WebApp.h
    tls/in6/WebApp.h
    tls/rc/WebApp.h
    ${JSONMIDDLEWARE_H}
)

add_library(
    http-server-express SHARED ${HTTP_SERVER_EXPRESS_CPP}
                               ${HTTP_SERVER_EXPRESS_H}
)
add_library(snodec::http-server-express ALIAS http-server-express)

target_include_directories(
    http-server-express PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                               "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    http-server-express PUBLIC http-server ${NLOHMANN_JSON_LIBRARIES}
)

get_target_property(HTTP_COMPILE_LIBDIR http-server HTTP_COMPILE_LIBDIR)
get_target_property(HTTP_INSTALL_LIBDIR http-server HTTP_INSTALL_LIBDIR)
get_target_property(HTTP_INSTALL_RPATH http-server HTTP_INSTALL_RPATH)

set_target_properties(
    http-server-express
    PROPERTIES OUTPUT_NAME snodec-http-server-express
               LIBRARY_OUTPUT_DIRECTORY ${HTTP_COMPILE_LIBDIR}
               SOVERSION ${SNODEC_SOVERSION}
               INSTALL_RPATH "${HTTP_INSTALL_RPATH}"
)

install(
    TARGETS http-server-express
    EXPORT snodec_http-server-express_Targets
    LIBRARY DESTINATION ${HTTP_INSTALL_LIBDIR} COMPONENT http-server-express
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/express
    COMPONENT http-server-express
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_http-server-express_Targets
    FILE snodec_http-server-express_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT http-server-express
)
