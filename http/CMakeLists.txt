cmake_minimum_required(VERSION 3.0)

add_library(http SHARED HTTPServer.cpp HTTPContext.cpp HTTPStatusCodes.cpp MimeTypes.cpp Request.cpp Response.cpp)

target_link_libraries(http LINK_PUBLIC net magic)

target_include_directories (http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
