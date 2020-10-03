cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

find_package(LibMagic REQUIRED)

add_library(http SHARED
    legacy/Client.h
    tls/Client.h
    legacy/Server.h
    tls/Server.h
    Client.h
    ClientContext.cpp
    ClientContext.h
    CookieOptions.h
    Parser.cpp
    Parser.h
    RequestParser.cpp
    RequestParser.h
    ResponseParser.cpp
    ResponseParser.h
    Server.h
    ServerContext.cpp
    ServerContext.h
    ServerRequest.cpp
    ServerRequest.h
    ServerResponse.cpp
    ServerResponse.h
    StatusCodes.cpp
    StatusCodes.h
    MimeTypes.cpp
    MimeTypes.h
    Request.cpp
    Request.h
    Response.cpp
    Response.h
    http_utils.cpp
    http_utils.h
)

target_link_libraries(http LINK_PUBLIC net logger utils ${LIBMAGIC_LIBRARIES})

target_compile_options(http PRIVATE --warn-all)

set_target_properties(http PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

FILE(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

set_target_properties(http PROPERTIES PUBLIC_HEADER "${includes}")

target_include_directories(http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBMAGIC_INCLUDES})

install(TARGETS http LIBRARY DESTINATION lib NAMELINK_COMPONENT libhttp.so.1)

if(CHECK_INCLUDES)
    set_property(TARGET http PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options})
endif(CHECK_INCLUDES)
