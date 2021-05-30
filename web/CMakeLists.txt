cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(LibMagic REQUIRED)

configure_file(config.h.in config.h @ONLY)

add_library(
    web SHARED
    http/Parser.cpp
    http/StatusCodes.cpp
    http/MimeTypes.cpp
    http/http_utils.cpp
    http/client/ResponseParser.cpp
    http/client/Request.cpp
    http/client/Response.cpp
    http/server/RequestParser.cpp
    http/server/Request.cpp
    http/server/Response.cpp
    ws/WSContext.cpp
    ws/WSReceiver.cpp
    ws/WSSubProtocol.cpp
    ws/WSTransmitter.cpp
    ws/ws_utils.cpp
    ws/client/WSContext.cpp
    ws/server/WSContext.cpp
    ws/subprotocol/WSSubProtocolSelector.cpp
)

add_subdirectory(ws/subprotocol)

target_link_libraries(web LINK_PUBLIC net ${LIBMAGIC_LIBRARIES})

target_compile_options(web PRIVATE --warn-all)

set_target_properties(
    web
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

target_include_directories(
    web PUBLIC ${PROJECT_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${LIBMAGIC_INCLUDES}
)

install(TARGETS web LIBRARY DESTINATION lib NAMELINK_COMPONENT libweb.so.1)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/web
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

if(CHECK_INCLUDES)
    set_property(
        TARGET web PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
