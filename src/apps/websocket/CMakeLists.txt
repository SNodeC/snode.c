# snode.c - a slim toolkit for network communication Copyright (C) 2020, 2021,
# 2022, 2023 Volker Christian <me@vchrist.at>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.18)

set(WSECHOSERVER_CPP echoserver.cpp)

set(WSECHOSERVER_H)

add_executable(wsechoserver ${WSECHOSERVER_CPP} ${WSECHOSERVER_H})
target_compile_definitions(
    wsechoserver PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(
    wsechoserver PUBLIC http-server-express net-in-stream-legacy
                        net-in-stream-tls
)

get_target_property(HTTP_INSTALL_RPATH http-server HTTP_INSTALL_RPATH)

set_target_properties(
    wsechoserver PROPERTIES INSTALL_RPATH
                            "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS wsechoserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                     COMPONENT apps
)

set(WSECHOCLIENT_CPP echoclient.cpp)

set(WSECHOCLIENT_H)

add_executable(wsechoclient ${WSECHOCLIENT_CPP} ${WSECHOCLIENT_H})
target_link_libraries(
    wsechoclient PRIVATE http-client net-in-stream-legacy net-in-stream-tls
)

get_target_property(HTTP_INSTALL_RPATH http-client HTTP_INSTALL_RPATH)

set_target_properties(
    wsechoclient PROPERTIES INSTALL_RPATH
                            "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS wsechoclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                     COMPONENT apps
)

add_subdirectory(subprotocol)
