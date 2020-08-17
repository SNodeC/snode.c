cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

find_package(LibMagic REQUIRED)

add_library(http SHARED
    legacy/HTTPServer.cpp
    legacy/HTTPServer.h
    tls/HTTPServer.cpp
    tls/HTTPServer.h
    HTTPParser.cpp
    HTTPParser.h
    HTTPRequestParser.cpp
    HTTPRequestParser.h
    HTTPServerContext.cpp
    HTTPServerContext.h
    HTTPStatusCodes.cpp
    HTTPStatusCodes.h
    MimeTypes.cpp
    MimeTypes.h
    Request.cpp
    Request.h
    Response.cpp
    Response.h
    httputils.cpp
    httputils.h
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
    set_property(TARGET http PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)
