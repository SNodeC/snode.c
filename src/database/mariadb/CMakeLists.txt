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

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBMARIADB libmariadb)

if(LIBMARIADB_FOUND)
    set(MARIADB_CPP
        MariaDBClient.cpp
        MariaDBClientASyncAPI.cpp
        MariaDBClientSyncAPI.cpp
        MariaDBCommand.cpp
        MariaDBCommandASync.cpp
        MariaDBCommandSync.cpp
        MariaDBCommandSequence.cpp
        MariaDBConnection.cpp
        commands/sync/MariaDBAffectedRowsCommand.cpp
        commands/sync/MariaDBFieldCountCommand.cpp
        commands/sync/MariaDBUseResultCommand.cpp
        commands/async/MariaDBAutoCommitCommand.cpp
        commands/async/MariaDBCommitCommand.cpp
        commands/async/MariaDBConnectCommand.cpp
        commands/async/MariaDBExecCommand.cpp
        commands/async/MariaDBFetchRowCommand.cpp
        commands/async/MariaDBFreeResultCommand.cpp
        commands/async/MariaDBQueryCommand.cpp
        commands/async/MariaDBRollbackCommand.cpp
    )

    set(MARIADB_H
        MariaDBClient.h
        MariaDBClientASyncAPI.h
        MariaDBClientSyncAPI.h
        MariaDBCommand.h
        MariaDBCommandASync.h
        MariaDBCommandSync.h
        MariaDBCommandSequence.h
        MariaDBConnection.h
        MariaDBConnectionDetails.h
        commands/sync/MariaDBAffectedRowsCommand.h
        commands/sync/MariaDBFieldCountCommand.h
        commands/sync/MariaDBUseResultCommand.h
        commands/async/MariaDBAutoCommitCommand.h
        commands/async/MariaDBCommitCommand.h
        commands/async/MariaDBConnectCommand.h
        commands/async/MariaDBExecCommand.h
        commands/async/MariaDBFetchRowCommand.h
        commands/async/MariaDBFreeResultCommand.h
        commands/async/MariaDBQueryCommand.h
        commands/async/MariaDBRollbackCommand.h
    )

    add_library(db-mariadb SHARED ${MARIADB_CPP} ${MARIADB_H})
    add_library(snodec::db-mariadb ALIAS db-mariadb)

    target_link_libraries(db-mariadb PUBLIC core ${LIBMARIADB_LIBRARIES})

    target_include_directories(
        db-mariadb
        PUBLIC ${LIBMARIADB_INCLUDE_DIRS}
               "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
               "$<INSTALL_INTERFACE:include/snode.c>"
    )

    set_target_properties(
        db-mariadb
        PROPERTIES VERSION ${SNode.C_VERSION}
                   SOVERSION ${SNODEC_SOVERSION}
                   OUTPUT_NAME snodec-db-mariadb
    )

    install(
        TARGETS db-mariadb
        EXPORT snodec_db-mariadb_Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT db-mariadb
    )

    install(
        EXPORT snodec_db-mariadb_Targets
        FILE snodec_db-mariadb_Targets.cmake
        NAMESPACE snodec::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/snodec
        COMPONENT db-mariadb
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/snode.c/database/mariadb
        COMPONENT db-mariadb
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        PATTERN "cmake" EXCLUDE
    )

endif(LIBMARIADB_FOUND)
