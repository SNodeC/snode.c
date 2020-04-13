cmake_minimum_required(VERSION 3.0)

project(servervoc)

add_subdirectory(net)

add_executable(servervoc main.cpp)

target_link_libraries(servervoc LINK_PUBLIC net)

install(TARGETS servervoc RUNTIME DESTINATION bin)
