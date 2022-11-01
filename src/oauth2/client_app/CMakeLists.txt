cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(client_app ClientApp.cpp)
target_link_libraries(
    client_app PRIVATE snodec::express snodec::net-in snodec::core-legacy
)
