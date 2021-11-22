cmake_minimum_required(VERSION 3.0)

add_library(echo STATIC EchoSocketContext.cpp)
target_include_directories(echo PRIVATE ${PROJECT_SOURCE_DIR})
