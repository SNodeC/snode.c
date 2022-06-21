cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(nlohmann_json 3.7.0)

if(NLOHMANN_JSON_FOUND)
    set(JSONMIDDLEWARE_CPP middleware/JsonMiddleware.cpp)
    set(JSONMIDDLEWARE_H middleware/JsonMiddleware.h)
    set(JSON_LIBRARY nlohmann_json::nlohmann_json)
else(NLOHMANN_JSON_FOUND)
    message(
        WARNING
            " nlohmann-json3-dev not found:\n"
            "    nlohmann_json is used to build the JsonMiddleware. In case you need\n"
            "   json support install nlohmann-json-dev"
    )
endif(NLOHMANN_JSON_FOUND)

set(NLOHMANN_JSON_FOUND
    ${NLOHMANN_JSON_FOUND}
    CACHE INTERNAL "nlohmann found?" FORCE
)

set(EXPRESS_CPP
    Request.cpp
    Response.cpp
    Router.cpp
    WebApp.cpp
    dispatcher/ApplicationDispatcher.cpp
    dispatcher/MiddlewareDispatcher.cpp
    dispatcher/MountPoint.cpp
    dispatcher/Route.cpp
    dispatcher/RootRoute.cpp
    dispatcher/RouterDispatcher.cpp
    dispatcher/State.cpp
    dispatcher/regex_utils.cpp
    middleware/BasicAuthentication.cpp
    middleware/StaticMiddleware.cpp
    middleware/VHost.cpp
    ${JSONMIDDLEWARE_CPP}
)

set(EXPRESS_H
    Request.h
    Response.h
    Router.h
    WebApp.h
    WebAppT.h
    dispatcher/ApplicationDispatcher.h
    dispatcher/Dispatcher.h
    dispatcher/MiddlewareDispatcher.h
    dispatcher/MountPoint.h
    dispatcher/Route.h
    dispatcher/RootRoute.h
    dispatcher/RouterDispatcher.h
    dispatcher/State.h
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

add_library(express SHARED ${EXPRESS_CPP} ${EXPRESS_H})
add_library(snodec::express ALIAS express)

target_include_directories(
    express PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                   "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(
    express
    PUBLIC snodec::httpserver
    PRIVATE ${JSON_LIBRARY}
)

set_target_properties(express PROPERTIES SOVERSION 1 OUTPUT_NAME snodec-express)

install(
    TARGETS express
    EXPORT snodec_express_Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c/express
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_express_Targets
    FILE snodec_express_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
