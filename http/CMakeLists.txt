cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

find_package(LibMagic REQUIRED)

add_library(http SHARED
    ClientContext.cpp
    Parser.cpp
    RequestParser.cpp
    ResponseParser.cpp
    ServerRequest.cpp
    ServerResponse.cpp
    StatusCodes.cpp
    MimeTypes.cpp
    Request.cpp
    Response.cpp
    http_utils.cpp
)

target_link_libraries(http LINK_PUBLIC net logger utils ${LIBMAGIC_LIBRARIES})

target_compile_options(http PRIVATE --warn-all)

set_target_properties(http PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

target_include_directories(http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBMAGIC_INCLUDES})

install(TARGETS http LIBRARY DESTINATION lib NAMELINK_COMPONENT libhttp.so.1)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/http
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "cmake" EXCLUDE
)

if(CHECK_INCLUDES)
    set_property(TARGET http
        PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
