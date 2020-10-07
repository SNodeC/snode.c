cmake_minimum_required(VERSION 3.0)

add_library(utils INTERFACE)

target_sources(utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/AttributeInjector.h)

target_include_directories(utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION include/snode.c/utils FILES_MATCHING PATTERN "*.h" PATTERN "cmake" EXCLUDE)
