cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(nlohmann_json 3.7.0 REQUIRED)

add_library(
    express SHARED
    Response.cpp
    Request.cpp
    Router.cpp
    WebApp.cpp
    middleware/JsonMiddleware.cpp
    middleware/StaticMiddleware.cpp
    middleware/VHost.cpp
)
add_library(snodec::express ALIAS express)

target_link_libraries(express PUBLIC snodec::utils snodec::logger snodec::web)
target_link_libraries(express PRIVATE nlohmann_json::nlohmann_json)

set_target_properties(
    express
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

target_include_directories(
    express PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
                   "$<INSTALL_INTERFACE:include/snode.c>"
)

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
