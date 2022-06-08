cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(authorization_server AuthorizationServer.cpp)
target_link_libraries(authorization_server PRIVATE snodec::express snodec::net-in snodec::core-legacy snodec::db-mariadb)
