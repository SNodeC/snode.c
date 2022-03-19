/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
 *               2021, 2022 Daniel Flockert
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/SNodeC.h"
#include "database/mariadb/MariaDBClient.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <mysql.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    database::mariadb::MariaDBConnectionDetails details = {
        .hostname = "localhost",
        .username = "root",
        .password = "pentium5",
        .database = "amarok",
        .port = 3306,
        .socket = "/run/mysqld/mysqld.sock",
        .flags = 0,
    };
    database::mariadb::MariaDBClient db1(details);

    db1.query(
        "select * from admin",
        [](void) -> void {
            VLOG(0) << "OnQuery";
        },
        [](const std::string& errorString) -> void {
            VLOG(0) << "Error: " << errorString;
        });

    database::mariadb::MariaDBClient db2(details);
    db2.query(
        "select * from admin",
        [](void) -> void {
            VLOG(0) << "OnQuery";
        },
        [](const std::string& errorString) -> void {
            VLOG(0) << "Error: " << errorString;
        });

    db2.query(
        "select * from admin",
        [](void) -> void {
            VLOG(0) << "OnQuery";
        },
        [](const std::string& errorString) -> void {
            VLOG(0) << "Error: " << errorString;
        });

    return core::SNodeC::start();
}

/*
string hostname = "localhost";
string username = "rathalin";
string password = "rathalin";
unsigned int port = 3306;
string socket_name = "/run/mysqld/mysqld.sock";
string db_name = "snodec";
unsigned int flags = 0;
*/
