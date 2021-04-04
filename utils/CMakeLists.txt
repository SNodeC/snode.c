cmake_minimum_required(VERSION 3.0)

add_library(utils SHARED Timeval.cpp instrumentation.cpp)

set_target_properties(
    utils
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED ON
               CXX_EXTENSIONS OFF
               SOVERSION 1
)

target_include_directories(utils PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(utils PUBLIC dl)

install(TARGETS utils LIBRARY DESTINATION lib NAMELINK_COMPONENT libutils.so.1)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snode.c/utils
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "cmake" EXCLUDE
)
