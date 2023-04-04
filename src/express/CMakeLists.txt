cmake_minimum_required(VERSION 3.18)

find_package(nlohmann_json 3.7.0)

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

set(NLOHMANN_JSON_FOUND
    ${NLOHMANN_JSON_FOUND}
    CACHE INTERNAL "nlohmann found?" FORCE
)

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
    http-server-express
    PUBLIC snodec::http-server
    PRIVATE ${NLOHMANN_JSON_LIBRARIES}
)

set_target_properties(
    http-server-express
    PROPERTIES SOVERSION 1
               OUTPUT_NAME snodec-http-server-express
#               INSTALL_RPATH "${SNODEC_WEB_HTTP_RPATH}"
)

install(
    TARGETS http-server-express
    EXPORT snodec_http-server-express_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/snode.c/web
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/express
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
)
