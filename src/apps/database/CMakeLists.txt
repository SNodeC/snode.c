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

if(LIBMARIADB_FOUND)

    add_executable(testmariadb testmariadb.cpp)
    target_link_libraries(testmariadb PRIVATE snodec::db-mariadb)
    target_include_directories(
        testmariadb
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${MARIADB_INCLUDE_DIRS}
               "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )
    install(TARGETS testmariadb RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                        COMPONENT apps
    )

    add_executable(
        test_mysql_real_connect_start test_mysql_real_connect_start.cpp
    )
    target_link_libraries(
        test_mysql_real_connect_start PRIVATE ${LIBMARIADB_LIBRARIES}
    )
    target_include_directories(
        test_mysql_real_connect_start
        PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${LIBMARIADB_INCLUDE_DIRS}
               "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )

    install(TARGETS test_mysql_real_connect_start
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT apps
    )

endif(LIBMARIADB_FOUND)
