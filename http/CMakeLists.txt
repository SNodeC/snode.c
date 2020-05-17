cmake_minimum_required(VERSION 3.0)

add_library(http SHARED HTTPServer.cpp HTTPContext.cpp HTTPStatusCodes.cpp MimeTypes.cpp Request.cpp Response.cpp httputils.cpp Router.cpp)

target_link_libraries(http LINK_PUBLIC net magic)

target_include_directories (http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(http PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
)

#find_program(iwyu_path NAMES include-what-you-use iwyu)
#if(NOT iwyu_path)
#  message(FATAL_ERROR "Could not find the program include-what-you-use")
#endif()
#set_property(TARGET http PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
