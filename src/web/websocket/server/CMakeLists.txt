cmake_minimum_required(VERSION 3.18)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(DISABLED_WARNINGS "-Wno-weak-template-vtables")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        set(DISABLED_WARNINGS
            "${DISABLED_WARNINGS} -Wno-cast-function-type-strict"
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

get_target_property(
    HTTP_SERVER_UPGRADE_COMPILE_LIBDIR snodec::http-server
    HTTP_SERVER_UPGRADE_COMPILE_LIBDIR
)

get_target_property(
    HTTP_SERVER_UPGRADE_INSTALL_LIBDIR snodec::http-server
    HTTP_SERVER_UPGRADE_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR snodec::websocket
    WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR snodec::websocket
    WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
)

set(WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR
    "${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}/server"
)

set(WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR
    "${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}/server"
)

set(WEBSOCKETSERVER_CPP
    GroupsManager.cpp SocketContextUpgrade.cpp SocketContextUpgradeFactory.cpp
    SubProtocol.cpp SubProtocolFactorySelector.cpp
)

set(WEBSOCKETSERVER_H
    GroupsManager.h SocketContextUpgrade.h SocketContextUpgradeFactory.h
    SubProtocol.h SubProtocolFactorySelector.h
)

add_library(websocket-server SHARED ${WEBSOCKETSERVER_CPP} ${WEBSOCKETSERVER_H})
add_library(snodec::websocket-server ALIAS websocket-server)

set_target_properties(
    websocket-server
    PROPERTIES
        OUTPUT_NAME "snodec-websocket-server"
        LIBRARY_OUTPUT_DIRECTORY "${HTTP_SERVER_UPGRADE_COMPILE_LIBDIR}"
        BUILD_RPATH "${WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR}"
        INSTALL_RPATH
        "${CMAKE_INSTALL_PREFIX}/${HTTP_UPGRADE_INSTALL_LIBDIR};${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR}"
        SOVERSION 1 # PREFIX "sc"
        WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR
        "${WEBSOCKET_SUBPROTOCOL_SERVER_COMPILE_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR
        "${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_SERVER_INCLUDE_DIRS
        "${CMAKE_INSTALL_PREFIX}/include/snode.c"
        EXPORT_PROPERTIES
        "WEBSOCKET_SUBPROTOCOL_SERVER_INCLUDE_DIRS;WEBSOCKET_SUBPROTOCOL_SERVER_INSTALL_LIBDIR"
        COMPILE_FLAGS "${DISABLED_WARNINGS}"
)

target_include_directories(
    websocket-server
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(websocket-server PUBLIC snodec::websocket)

install(
    TARGETS websocket-server
    EXPORT snodec_websocket-server_Targets
    LIBRARY DESTINATION ${HTTP_SERVER_UPGRADE_INSTALL_LIBDIR}
            COMPONENT websocket-server
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/websocket/server
    COMPONENT websocket-server
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_websocket-server_Targets
    FILE snodec_websocket-server_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
    COMPONENT websocket-server
)

# HTTP_SERVER_UPGRADE_COMPILE_LIBDIR   // Compile to
# HTTP_SERVER_UPGRADE_INSTALL_LIBDIR   // Install to

# WEBSOCKET_SERVER_SUBPROTOCOL_COMPILE_LIBDIR   // RPATH to | Subprotocol server
# libraries Compile to WEBSOCKET_SERVER_SUBPROTOCOL_INSTALL_LIBDIR   // RPATH to
# | Subprotocol server libraries Install to
