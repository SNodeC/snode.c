# SNode.C - a slim toolkit for network communication
# Copyright (C) Volker Christian <me@vchrist.at>
#               2020, 2021, 2022, 2023, 2024, 2025
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

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(client_app ClientApp.cpp)

target_link_libraries(
    client_app PRIVATE http-server-express net-in-stream-legacy
)

if(SNODEC_INTREE_BUILD)

    set_target_properties(
        client_app PROPERTIES INSTALL_RPATH
                              "${CMAKE_INSTALL_RPATH};${HTTP_INSTALL_RPATH}"
    )

endif(SNODEC_INTREE_BUILD)

install(TARGETS client_app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                   COMPONENT apps
)
