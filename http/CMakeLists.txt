cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

find_package(LibMagic REQUIRED)

add_library(http SHARED 
    WebApp.cpp
    HTTPContext.cpp
    HTTPStatusCodes.cpp
    MimeTypes.cpp
    Request.cpp
    Response.cpp
    httputils.cpp
    Router.cpp
)

target_link_libraries(http LINK_PUBLIC net ${LIBMAGIC_LIBRARIES})

set_target_properties(http PROPERTIES
            CXX_STANDARD 17
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
