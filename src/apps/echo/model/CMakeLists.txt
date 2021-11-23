cmake_minimum_required(VERSION 3.0)

add_library(echosocketcontext STATIC EchoSocketContext.cpp)
target_include_directories(echosocketcontext PRIVATE ${PROJECT_SOURCE_DIR})
