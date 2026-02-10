# SNode.C - A Slim Toolkit for Network Communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

add_compile_options($<$<CXX_COMPILER_ID:Clang>:-Wno-missing-prototypes>)
add_compile_options($<$<CXX_COMPILER_ID:Clang>:-Wno-weak-vtables>)

get_target_property(HTTP_INSTALL_RPATH http-server HTTP_INSTALL_RPATH)
get_target_property(HTTP_INSTALL_RPATH http-client HTTP_INSTALL_RPATH)

add_executable(snode.c main.cpp)
target_link_libraries(
    snode.c PUBLIC core http-server-express net-in-stream-legacy
)
set_target_properties(
    snode.c PROPERTIES INSTALL_RPATH
                       "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS snode.c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                COMPONENT apps
)

add_executable(express-compat-server express_compat_server.cpp)
target_link_libraries(
    express-compat-server PUBLIC core http-server-express net-in-stream-legacy
)
set_target_properties(
    express-compat-server
    PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS express-compat-server
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
)

add_executable(testpost testpost.cpp)
target_link_libraries(
    testpost PRIVATE http-server-express net-in-stream-legacy net-in-stream-tls
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
    verysimpleserver PRIVATE http-server-express net-in-stream-legacy
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
        jsonserver PRIVATE http-server-express net-in-stream-legacy
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
target_link_libraries(jsonclient PRIVATE http-client net-in-stream-legacy)
set_target_properties(
    jsonclient PROPERTIES INSTALL_RPATH
                          "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
)
install(TARGETS jsonclient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                   COMPONENT apps
)

if(LIBMARIADB_FOUND)
    add_executable(testregex testregex.cpp)
    target_link_libraries(
        testregex PRIVATE http-server-express net-in-stream-legacy
                          net-in-stream-tls db-mariadb
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
    warema-jalousien PRIVATE http-server-express net-in-stream-legacy
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
    vhostserver PRIVATE http-server-express net-in6-stream-legacy
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
    testbasicauthentication PRIVATE http-server-express net-in6-stream-legacy
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
