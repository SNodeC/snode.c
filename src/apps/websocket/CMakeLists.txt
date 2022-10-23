cmake_minimum_required(VERSION 3.0)

set(WSECHOSERVER_CPP echoserver.cpp)

set(WSECHOSERVER_H)

add_executable(wsechoserver ${WSECHOSERVER_CPP} ${WSECHOSERVER_H})
target_compile_definitions(
    wsechoserver PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
    wsechoserver PRIVATE snodec::http-server-express
                         snodec::net-in-stream-legacy snodec::net-in-stream-tls
)
set_target_properties(
    wsechoserver PROPERTIES INSTALL_RPATH "${SNODEC_WEB_HTTP_RPATH}"
)
install(TARGETS wsechoserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(WSECHOCLIENT_CPP echoclient.cpp)

set(WSECHOCLIENT_H)

add_executable(wsechoclient ${WSECHOCLIENT_CPP} ${WSECHOCLIENT_H})
target_link_libraries(
    wsechoclient PRIVATE snodec::http-client snodec::net-in-stream-legacy
                         snodec::net-in-stream-tls
)
set_target_properties(
    wsechoclient PROPERTIES INSTALL_RPATH "${SNODEC_WEB_HTTP_RPATH}"
)
install(TARGETS wsechoclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(subprotocol)
