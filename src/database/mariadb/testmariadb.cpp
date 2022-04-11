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
#include "core/timer/Timer.h"
#include "database/mariadb/MariaDBClient.h"
#include "database/mariadb/MariaDBConnectionDetails.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    database::mariadb::MariaDBConnectionDetails details = {
        .hostname = "localhost",
        .username = "snodec",
        .password = "pentium5",
        .database = "snodec",
        .port = 3306,
        .socket = "/run/mysqld/mysqld.sock",
        .flags = 0,
    };

    database::mariadb::MariaDBClient db1(details);
    /*
        db1.insert(
            "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','Hallo')",
            [](void) -> void {
                VLOG(0) << "OnQuery";
            },
            [](const std::string& errorString) -> void {
                VLOG(0) << "Error: " << errorString;
            });
    */

    db1.query(
        "DELETE FROM `snodec`",
        [](void) -> void {
            VLOG(0) << "OnQuery";
        },
        [](const std::string& errorString) -> void {
            VLOG(0) << "Error: " << errorString;
        });

    database::mariadb::MariaDBClient db2(details);
    {
        db2.query(
            "select * from snodec",
            [](void) -> void {
                VLOG(0) << "OnQuery 1";
            },
            [](const std::string& errorString) -> void {
                VLOG(0) << "Error 1: " << errorString;
            });

        db2.query(
            "select * from snodec",
            [&db2](void) -> void {
                VLOG(0) << "OnQuery 2";
                db2.query(
                    "select * from snodec",
                    [&db2](void) -> void {
                        VLOG(0) << "OnQuery 3";

                        core::timer::Timer dbTimer1 = core::timer::Timer::intervalTimer(
                            [&db2](const void* arg, [[maybe_unused]] const std::function<void()>& stop) -> void {
                                static int i = 0;
                                std::cout << static_cast<const char*>(arg) << " " << i++ << std::endl;

                                db2.query(
                                    "select * from snodec",
                                    [](void) -> void {
                                        VLOG(0) << "OnQuery 4";
                                    },
                                    [stop](const std::string& errorString) -> void {
                                        VLOG(0) << "Error 4: " << errorString;
                                        stop();
                                    });
                            },
                            0.5,
                            "Tick 2");

                        core::timer::Timer dbTimer2 = core::timer::Timer::intervalTimer(
                            [&db2](const void* arg, [[maybe_unused]] const std::function<void()>& stop) -> void {
                                static int i = 0;
                                std::cout << static_cast<const char*>(arg) << " " << i++ << std::endl;

                                db2.query(
                                    "select * from snodec",
                                    [](void) -> void {
                                        VLOG(0) << "OnQuery 5";
                                    },
                                    [stop](const std::string& errorString) -> void {
                                        VLOG(0) << "Error 5: " << errorString;
                                        stop();
                                    });
                            },
                            0.7,
                            "Tick 0.7");
                    },
                    [](const std::string& errorString) -> void {
                        VLOG(0) << "Error 3: " << errorString;
                    });
            },
            [](const std::string& errorString) -> void {
                VLOG(0) << "Error 2: " << errorString;
            });

        core::timer::Timer dbTimer = core::timer::Timer::intervalTimer(
            [&db2](const void* arg, [[maybe_unused]] const std::function<void()>& stop) -> void {
                static int i = 0;
                std::cout << static_cast<const char*>(arg) << " " << i++ << std::endl;

                db2.insert(
                    "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','Hallo')",
                    [](void) -> void {
                        VLOG(0) << "OnQuery 6";
                    },
                    [stop](const std::string& errorString) -> void {
                        VLOG(0) << "Error 6: " << errorString;
                        stop();
                    });
            },
            1.5,
            "Tick 1.5");
    }

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
