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
    WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR
)

get_target_property(
    WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR snodec::websocket-client
    WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR
)

get_target_property(
    HTTP_UPGRADE_INSTALL_LIBDIR snodec::http-client HTTP_UPGRADE_INSTALL_LIBDIR
)

set(ECHOCLIENTSUBPROTOCOL_CPP Echo.cpp EchoFactory.cpp)

set(ECHOCLIENTSUBPROTOCOL_H Echo.h EchoFactory.h)

add_library(
    websocket-echo-client SHARED ${ECHOCLIENTSUBPROTOCOL_CPP}
                                 ${ECHOCLIENTSUBPROTOCOL_H}
)

target_include_directories(
    websocket-echo-client PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)

target_link_libraries(websocket-echo-client PUBLIC snodec::websocket-client)

set_target_properties(
    websocket-echo-client
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
               "${WEBSOCKET_SUBPROTOCOL_COMPILE_LIBDIR}"
               OUTPUT_NAME "snodec-websocket-echo-client"
               SOVERSION ${SNODEC_SOVERSION} # PREFIX "ssp"
               INSTALL_RPATH
               "${CMAKE_INSTALL_PREFIX}/${HTTP_UPGRADE_INSTALL_LIBDIR}"
)

install(TARGETS websocket-echo-client
        LIBRARY DESTINATION ${WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR}
                COMPONENT apps
)

if(CHECK_INCLUDES)
    set_property(
        TARGET websocket-echo-client PROPERTY CXX_INCLUDE_WHAT_YOU_USE
                                              ${iwyu_path_and_options}
    )
endif(CHECK_INCLUDES)
