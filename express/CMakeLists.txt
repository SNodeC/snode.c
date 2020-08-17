cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

find_package(LibMagic REQUIRED)

add_library(express SHARED
    Router.cpp
    Router.h
    WebApp.cpp
    WebApp.h
    legacy/WebApp.cpp
    legacy/WebApp.h
    tls/WebApp.cpp
    tls/WebApp.h
    middleware/StaticMiddleware.cpp
    middleware/StaticMiddleware.h
)

target_link_libraries(express LINK_PUBLIC http net logger utils ${LIBMAGIC_LIBRARIES})

target_compile_options(express PRIVATE --warn-all)

set_target_properties(express PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            SOVERSION 1
)

FILE(GLOB includes "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

set_target_properties(express PROPERTIES PUBLIC_HEADER "${includes}")

target_include_directories(express PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBMAGIC_INCLUDES})

install(TARGETS http LIBRARY DESTINATION lib NAMELINK_COMPONENT libhttp.so.1)

if(CHECK_INCLUDES)
    set_property(TARGET http PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif(CHECK_INCLUDES)
