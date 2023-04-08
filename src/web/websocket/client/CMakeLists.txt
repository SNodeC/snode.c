cmake_minimum_required(VERSION 3.18)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-weak-template-vtables)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    # using Intel C++
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # using Visual Studio C++
endif()

get_target_property(
    HTTP_CLIENT_UPGRADE_COMPILE_LIBDIR snodec::http-client
    HTTP_CLIENT_UPGRADE_COMPILE_LIBDIR
)

get_target_property(
    HTTP_CLIENT_UPGRADE_INSTALL_LIBDIR snodec::http-client
    HTTP_CLIENT_UPGRADE_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR snodec::websocket
    WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR snodec::websocket
    WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
)

set(WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR
    "${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}/client"
)

set(WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR
    "${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}/client"
)

set(WEBSOCKETCLIENT_CPP
    SocketContextUpgrade.cpp SocketContextUpgradeFactory.cpp SubProtocol.cpp
    SubProtocolFactorySelector.cpp
)

set(WEBSOCKETCLIENT_H SocketContextUpgrade.h SocketContextUpgradeFactory.h
                      SubProtocol.h SubProtocolFactorySelector.h
)

add_library(websocket-client SHARED ${WEBSOCKETCLIENT_CPP} ${WEBSOCKETCLIENT_H})
add_library(snodec::websocket-client ALIAS websocket-client)

set_target_properties(
    websocket-client
    PROPERTIES
        OUTPUT_NAME "snodec-websocket-client"
        LIBRARY_OUTPUT_DIRECTORY "${HTTP_CLIENT_UPGRADE_COMPILE_LIBDIR}"
        SOVERSION 1 # PREFIX "sc"
        BUILD_RPATH "${WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR}"
        INSTALL_RPATH
        "${CMAKE_INSTALL_PREFIX}/${HTTP_UPGRADE_INSTALL_LIBDIR};${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR
        "${WEBSOCKET_SUBPROTOCOL_CLIENT_COMPILE_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR
        "${CMAKE_INSTALL_PREFIX}/${WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR}"
        WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS
        "${CMAKE_INSTALL_PREFIX}/include/snode.c"
        EXPORT_PROPERTIES
        "WEBSOCKET_SUBPROTOCOL_CLIENT_INCLUDE_DIRS;WEBSOCKET_SUBPROTOCOL_CLIENT_INSTALL_LIBDIR"
)

target_include_directories(
    websocket-client
    PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
           "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
           "$<INSTALL_INTERFACE:include/snode.c>"
)

target_link_libraries(websocket-client PUBLIC snodec::websocket)

install(
    TARGETS websocket-client
    EXPORT snodec_websocket-client_Targets
    LIBRARY DESTINATION ${HTTP_CLIENT_UPGRADE_INSTALL_LIBDIR}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web/websocket/client
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

install(
    EXPORT snodec_websocket-client_Targets
    FILE snodec_websocket-client_Targets.cmake
    NAMESPACE snodec::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
)

# HTTP_CLIENT_UPGRADE_COMPILE_LIBDIR   // Compile to
# HTTP_CLIENT_UPGRADE_INSTALL_LIBDIR   // Install to

# WEBSOCKET_CLIENT_SUBPROTOCOL_COMPILE_LIBDIR   // RPATH to | Subprotocol client
# libraries Compile to WEBSOCKET_CLIENT_SUBPROTOCOL_INSTALL_LIBDIR   // RPATH to
# | Subprotocol client libraries Install to
