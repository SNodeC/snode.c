cmake_minimum_required(VERSION 3.0)

project(snode.c)

add_subdirectory(net)

add_executable(snode.c main.cpp)

target_link_libraries(snode.c LINK_PUBLIC net)

install(TARGETS snode.c RUNTIME DESTINATION bin)
