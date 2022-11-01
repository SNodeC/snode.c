cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(resource_server ResourceServer.cpp)

target_link_libraries(
    resource_server PRIVATE snodec::http-server-express snodec::http-client
                            snodec::net-in-stream-legacy snodec::db-mariadb
)

set_target_properties(
    resource_server PROPERTIES INSTALL_RPATH "${SNODEC_WEB_HTTP_RPATH}"
)

install(TARGETS resource_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
