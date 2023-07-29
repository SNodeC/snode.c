cmake_minimum_required(VERSION 3.18)

set(WSECHOSERVER_CPP echoserver.cpp)

set(WSECHOSERVER_H)

add_executable(wsechoserver ${WSECHOSERVER_CPP} ${WSECHOSERVER_H})
target_compile_definitions(
    wsechoserver PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
    wsechoserver PUBLIC snodec::http-server-express
                        snodec::net-in-stream-legacy snodec::net-in-stream-tls
)

get_target_property(HTTP_RPATH snodec::http-server HTTP_RPATH)

set_target_properties(
    wsechoserver PROPERTIES INSTALL_RPATH
                            "${CMAKE_INSTALL_RPATH};${HTTP_RPATH}"
)
install(TARGETS wsechoserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                     COMPONENT apps
)

set(WSECHOCLIENT_CPP echoclient.cpp)

set(WSECHOCLIENT_H)

add_executable(wsechoclient ${WSECHOCLIENT_CPP} ${WSECHOCLIENT_H})
target_link_libraries(
    wsechoclient PRIVATE snodec::http-client snodec::net-in-stream-legacy
                         snodec::net-in-stream-tls
)

get_target_property(HTTP_RPATH snodec::http-client HTTP_RPATH)

set_target_properties(
    wsechoclient PROPERTIES INSTALL_RPATH
                            "${CMAKE_INSTALL_RPATH};${HTTP_RPATH}"
)
install(TARGETS wsechoclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                     COMPONENT apps
)

add_subdirectory(subprotocol)
