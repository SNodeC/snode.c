cmake_minimum_required(VERSION 3.0)

add_executable(websocketechoserver websocketechoserver.cpp)
target_compile_definitions(
    websocketechoserver
    PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(websocketechoserver PRIVATE snodec::express)
install(TARGETS websocketechoserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(websocketechoclient websocketechoclient.cpp)
target_link_libraries(websocketechoclient PRIVATE snodec::httpclient)
install(TARGETS websocketechoclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(subprotocol)
