cmake_minimum_required(VERSION 3.0)

set(SOCKETCONTEXTUPGRADE_SERVER_LIBDIR
    ${CMAKE_CURRENT_BINARY_DIR}
    CACHE INTERNAL "SOCKETCONTEXTUPGRADE_SERVER_LIBDIR"
)

configure_file(config.h.in config.h @ONLY)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(LibMagic REQUIRED)

add_library(
    web SHARED
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

target_link_libraries(
    web LINK_PUBLIC net::net snodec::utils ${LIBMAGIC_LIBRARIES}
)

set_target_properties(
    web
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

target_include_directories(
    web
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>" "${LIBMAGIC_INCLUDES}"
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/http
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    TARGETS web
    EXPORT webTargets
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION lib
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/snode.c
)

install(
    EXPORT webTargets
    FILE webTargets.cmake
    NAMESPACE web::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/web
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/webConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/web
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/webConfig.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/web
)

if(CHECK_INCLUDES)
    set_property(
        TARGET web PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
