# SNode.C - a slim toolkit for network communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024
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

add_subdirectory(model)

get_filename_component(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR} NAME)

set(NET ${CURRENT_DIR})

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

set(NETS "in" "in6" "un")
if(BLUEZ_FOUND)
    list(APPEND NETS "l2" "rc")
endif(BLUEZ_FOUND)

set(ECHOCLIENT_CPP echoclient.cpp)
set(ECHOCLIENT_H model/clients.h)

set(ECHOSERVER_CPP echoserver.cpp)
set(ECHOSERVER_H model/servers.h)

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
            echoclient-${STREAM}-${NET} ${ECHOCLIENT_CPP} ${ECHOCLIENT_H}
        )
        target_link_libraries(
            echoclient-${STREAM}-${NET} PRIVATE net-${NET}-stream-${STREAM}
                                                echosocketcontext
        )
        target_compile_definitions(
            echoclient-${STREAM}-${NET}
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS echoclient-${STREAM}-${NET}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
        )

        add_executable(
            echoserver-${STREAM}-${NET} ${ECHOSERVER_CPP} ${ECHOSERVER_H}
        )
        target_link_libraries(
            echoserver-${STREAM}-${NET} PRIVATE net-${NET}-stream-${STREAM}
                                                echosocketcontext
        )
        target_compile_definitions(
            echoserver-${STREAM}-${NET}
            PRIVATE NET=${NET} NET_TYPE=${NET_TYPE} STREAM=${STREAM}
                    STREAM_TYPE=${STREAM_TYPE}
        )
        install(TARGETS echoserver-${STREAM}-${NET}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
        )
    endforeach()
endforeach()
