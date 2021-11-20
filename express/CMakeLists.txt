cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(nlohmann_json 3.7.0 QUIET)

#if(NLOHMANN_JSON_FOUND)
#    set(JSON_MIDDLEWARE_SOURCES middleware/JsonMiddleware.cpp)
#    set(JSON_LIBRARY nlohmann_json::nlohmann_json)
#else(NLOHMANN_JSON_FOUND)
#    message(
#        WARNING
#            " nlohmann-json3-dev not found:\n"
#            "    nlohmann_json is used to build the JsonMiddleware. In case you need\n"
#            "    json support install nlohmann-json-dev"
#    )
#endif(NLOHMANN_JSON_FOUND)

set(NLOHMANN_JSON_FOUND
    ${NLOHMANN_JSON_FOUND}
    CACHE INTERNAL "nlohmann found?" FORCE
)

add_library(
    express SHARED
    Response.cpp
    Request.cpp
    Router.cpp
    WebApp.cpp
    ${JSON_MIDDLEWARE_SOURCES}
    middleware/StaticMiddleware.cpp
    middleware/VHost.cpp
)
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
    EXPORT snodecexpressTargets
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
    EXPORT snodecexpressTargets
    FILE snodecexpressTargets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)
