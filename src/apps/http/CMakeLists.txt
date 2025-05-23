# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# ---------------------------------------------------------------------------
#
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 3.18)

get_target_property(HTTP_INSTALL_RPATH http-server HTTP_INSTALL_RPATH)
get_target_property(HTTP_INSTALL_RPATH http-client HTTP_INSTALL_RPATH)

set(IN 1)
set(IN6 2)
set(L2 3)
set(RC 4)
set(UN 5)

set(LEGACY 1)
set(TLS 2)

add_compile_definitions(
    IN=${IN}
    IN6=${IN6}
    L2=${L2}
    RC=${RC}
    UN=${UN}
    LEGACY=${LEGACY}
    TLS=${TLS}
)

set(HTTPCLIENT_CPP httpclient.cpp)
set(HTTPCLIENT_H model/clients.h)

set(HTTPCLIENTCLIENTCERT_CPP httpclientclientcert.cpp)
set(HTTPCLIENTCLIENTCERT_H model/clients.h)

set(HTTPSERVER_CPP httpserver.cpp)
set(HTTPSERVER_H model/servers.h)

set(HTTPSERVERCLIENTCERT_CPP httpserverclientcert.cpp)
set(HTTPSERVERCLIENTCERT_H model/servers.h)

set(NETS "in" "in6" "un")
if(BLUEZ_FOUND)
    list(APPEND NETS "rc")
endif(BLUEZ_FOUND)

foreach(NET IN LISTS NETS)
    if(${NET} STREQUAL "in")
        set(NET_TYPE ${IN})
    elseif(${NET} STREQUAL "in6")
        set(NET_TYPE ${IN6})
    elseif(${NET} STREQUAL "l2")
        set(NET_TYPE ${L2})
    elseif(${NET} STREQUAL "rc")
        set(NET_TYPE ${RC})
    elseif(${NET} STREQUAL "un")
        set(NET_TYPE ${UN})
    endif()

    set(STREAMS "legacy" "tls")
    foreach(STREAM IN LISTS STREAMS)
        if(${STREAM} STREQUAL "legacy")
            set(STREAM_TYPE ${LEGACY})
        elseif(${STREAM} STREQUAL "tls")
            set(STREAM_TYPE ${TLS})
        endif()

        add_executable(
            httpserver-${STREAM}-${NET} ${HTTPSERVER_CPP} ${HTTPSERVER_H}
        )
        target_link_libraries(
            httpserver-${STREAM}-${NET} PUBLIC http-server-express
                                               net-${NET}-stream-${STREAM}
        )
        set_target_properties(
            httpserver-${STREAM}-${NET}
            PROPERTIES INSTALL_RPATH
                       "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
        )
        target_compile_definitions(
            httpserver-${STREAM}-${NET}
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS httpserver-${STREAM}-${NET}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
        )

        add_executable(
            httpclient-${STREAM}-${NET} ${HTTPCLIENT_CPP} ${HTTPCLIENT_H}
        )
        set_target_properties(
            httpclient-${STREAM}-${NET}
            PROPERTIES INSTALL_RPATH
                       "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
        )
        target_link_libraries(
            httpclient-${STREAM}-${NET} PUBLIC http-client
                                               net-${NET}-stream-${STREAM}
        )
        target_compile_definitions(
            httpclient-${STREAM}-${NET}
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS httpclient-${STREAM}-${NET}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
        )

        if(${STREAM} STREQUAL "tls")
            add_executable(
                httpserverclientcert-${STREAM}-${NET}
                ${HTTPSERVERCLIENTCERT_CPP} ${HTTPSERVERCLIENTCERT_H}
            )
            set_target_properties(
                httpserverclientcert-${STREAM}-${NET}
                PROPERTIES INSTALL_RPATH
                           "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
            )
            target_link_libraries(
                httpserverclientcert-${STREAM}-${NET}
                PUBLIC http-server-express net-${NET}-stream-${STREAM}
            )
            target_compile_definitions(
                httpserverclientcert-${STREAM}-${NET}
                PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                        STREAM_TYPE=${STREAM_TYPE}
            )
            install(TARGETS httpserverclientcert-${STREAM}-${NET}
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
            )

            add_executable(
                httpclientclientcert-${STREAM}-${NET}
                ${HTTPCLIENTCLIENTCERT_CPP} ${HTTPCLIENTCLIENTCERT_H}
            )
            set_target_properties(
                httpclientclientcert-${STREAM}-${NET}
                PROPERTIES INSTALL_RPATH
                           "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
            )
            target_link_libraries(
                httpclientclientcert-${STREAM}-${NET}
                PUBLIC http-client net-${NET}-stream-${STREAM}
            )
            target_compile_definitions(
                httpclientclientcert-${STREAM}-${NET}
                PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                        STREAM_TYPE=${STREAM_TYPE}
            )
            install(TARGETS httpclientclientcert-${STREAM}-${NET}
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
            )
        endif()
    endforeach()
endforeach()

add_executable(httplowlevelclient httplowlevelclient.cpp)
set_target_properties(
    httplowlevelclient
    PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
target_link_libraries(
    httplowlevelclient PUBLIC net-in-stream-legacy net-in-stream-tls
                              http-client
)
install(TARGETS httplowlevelclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                           COMPONENT apps
)
