cmake_minimum_required(VERSION 3.18)


set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH} "${CMAKE_INSTALL_PREFIX}/lib/snode.c/iot/mqtt" CACHE INTERNAL "")

add_subdirectory(mqtt)
