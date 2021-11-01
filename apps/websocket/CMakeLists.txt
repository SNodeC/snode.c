cmake_minimum_required(VERSION 3.0)

add_executable(echoserver echoserver.cpp)
target_compile_definitions(
    echoserver PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(echoserver PRIVATE snodec::express)
install(TARGETS echoserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(echoclient echoclient.cpp)
target_link_libraries(echoclient PRIVATE snodec::httpclient)
install(TARGETS echoclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(subprotocol)
