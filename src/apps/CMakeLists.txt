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

add_compile_options($<$<CXX_COMPILER_ID:Clang>:-Wno-missing-prototypes>)

get_target_property(HTTP_INSTALL_RPATH http-server HTTP_INSTALL_RPATH)
get_target_property(HTTP_INSTALL_RPATH http-client HTTP_INSTALL_RPATH)

add_executable(snode.c main.cpp)
target_link_libraries(snode.c PUBLIC core)
install(TARGETS snode.c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                COMPONENT apps
)

add_executable(testpost testpost.cpp)
target_link_libraries(
    testpost PRIVATE http-server-express net-in-stream-legacy
                     net-in-stream-tls
)
set_target_properties(
    testpost PROPERTIES INSTALL_RPATH
                        "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS testpost RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                 COMPONENT apps
)

add_executable(verysimpleserver verysimpleserver.cpp)
target_link_libraries(
    verysimpleserver
    PRIVATE http-server-express net-in-stream-legacy
            net-in-stream-tls
)
set_target_properties(
    verysimpleserver PROPERTIES INSTALL_RPATH
                                "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS verysimpleserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                         COMPONENT apps
)

if(NLOHMANN_JSON_FOUND)
    add_executable(jsonserver jsonserver.cpp)
    target_link_libraries(
        jsonserver PRIVATE http-server-express
                           net-in-stream-legacy
    )
    set_target_properties(
        jsonserver PROPERTIES INSTALL_RPATH
                              "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
    )
    install(TARGETS jsonserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                       COMPONENT apps
    )
endif(NLOHMANN_JSON_FOUND)

add_executable(jsonclient jsonclient.cpp)
target_link_libraries(
    jsonclient PRIVATE http-client net-in-stream-legacy
)
set_target_properties(
    jsonclient PROPERTIES INSTALL_RPATH
                          "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS jsonclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                   COMPONENT apps
)

add_executable(testparser testparsers.cpp)
target_link_libraries(
    testparser PRIVATE http-server http-client core
                       core-socket-stream logger
)
set_target_properties(
    testparser
    PROPERTIES
        INSTALL_RPATH
        "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS testparser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                   COMPONENT apps
)

if(LIBMARIADB_FOUND)
    add_executable(testregex testregex.cpp)
    target_link_libraries(
        testregex
        PRIVATE http-server-express net-in-stream-legacy
                net-in-stream-tls snodec::db-mariadb
    )
    set_target_properties(
        testregex PROPERTIES INSTALL_RPATH
                             "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
    )
    install(TARGETS testregex RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                      COMPONENT apps
    )
endif(LIBMARIADB_FOUND)

add_executable(warema-jalousien warema-jalousien.cpp)
target_link_libraries(
    warema-jalousien PRIVATE http-server-express
                             net-in-stream-legacy
)
set_target_properties(
    warema-jalousien PROPERTIES INSTALL_RPATH
                                "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS warema-jalousien RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                         COMPONENT apps
)

add_executable(vhostserver vhostserver.cpp)
target_link_libraries(
    vhostserver
    PRIVATE http-server-express net-in6-stream-legacy
            net-in6-stream-tls
)
set_target_properties(
    vhostserver PROPERTIES INSTALL_RPATH
                           "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS vhostserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                    COMPONENT apps
)

add_executable(testpipe testpipe.cpp)
target_link_libraries(testpipe PRIVATE core)
install(TARGETS testpipe RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                 COMPONENT apps
)

add_executable(configtest configtest.cpp)
target_link_libraries(configtest PRIVATE core)
install(TARGETS configtest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                   COMPONENT apps
)

add_executable(testbasicauthentication testbasicauthentication.cpp)
target_link_libraries(
    testbasicauthentication
    PRIVATE http-server-express net-in6-stream-legacy
            net-in6-stream-tls
)
set_target_properties(
    testbasicauthentication
    PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS testbasicauthentication
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
)

macro(subdirlist result curdir)
    file(
        GLOB children
        RELATIVE ${curdir}
        ${curdir}/*
    )
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${curdir}/${child}
           AND EXISTS ${curdir}/${child}/CMakeLists.txt
        )
            list(APPEND dirlist ${child})
        endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()

subdirlist(subdirs ${CMAKE_CURRENT_SOURCE_DIR})

foreach(subdir ${subdirs})
    add_subdirectory(${subdir})
endforeach()
