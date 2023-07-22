cmake_minimum_required(VERSION 3.18)

set(ECHOSOCKETCONTEXT_CPP EchoSocketContext.cpp)

set(ECHOSOCKETCONTEXT_H EchoSocketContext.h)

add_library(
    echosocketcontext STATIC ${ECHOSOCKETCONTEXT_CPP} ${ECHOSOCKETCONTEXT_H}
)
target_include_directories(echosocketcontext PRIVATE ${PROJECT_SOURCE_DIR})
