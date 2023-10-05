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

if(LIBMARIADB_FOUND)

    get_target_property(HTTP_INSTALL_RPATH http-server HTTP_INSTALL_RPATH)

    get_target_property(HTTP_INSTALL_RPATH http-client HTTP_INSTALL_RPATH)

    add_subdirectory(authorization_server)
    add_subdirectory(client_app)
    add_subdirectory(resource_server)

endif(LIBMARIADB_FOUND)
