# snode.c - a slim toolkit for network communication
# Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

cmake_minimum_required(VERSION 3.18)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR snodec::websocket-server
    WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR snodec::websocket-server
    WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
)

get_target_property(
    HTTP_UPGRADE_INSTALL_LIBDIR snodec::http-server HTTP_UPGRADE_INSTALL_LIBDIR
)

set(ECHOSERVERSUBPROTOCOL_CPP Echo.cpp EchoFactory.cpp)

set(ECHOSERVERSUBPROTOCOL_H Echo.h EchoFactory.h)

add_library(
    websocket-echo-server SHARED ${ECHOSERVERSUBPROTOCOL_CPP}
                                 ${ECHOSERVERSUBPROTOCOL_H}
)

target_include_directories(
    websocket-echo-server PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

target_link_libraries(websocket-echo-server PUBLIC snodec::websocket-server)

set_target_properties(
    websocket-echo-server
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}"
               OUTPUT_NAME "snodec-websocket-echo-server"
               SOVERSION ${SNODEC_SOVERSION} # PREFIX "ssp"
               INSTALL_RPATH
               "${CMAKE_INSTALL_PREFIX}/${HTTP_UPGRADE_INSTALL_LIBDIR}"
)

install(TARGETS websocket-echo-server
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}
                COMPONENT apps
)

if(CHECK_INCLUDES)
    set_property(
        TARGET websocket-echo-server PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                              ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
