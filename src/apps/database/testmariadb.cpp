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

#include <mysql.h>
#include <poll.h>
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

    db1.query(
        "DELETE FROM `snodec`",
        []([[maybe_unused]] const MYSQL_ROW row) -> void {
            VLOG(0) << "OnQuery";
        },
        [](const std::string& errorString, unsigned int errorNumber) -> void {
            VLOG(0) << "Error: " << errorString << " : " << errorNumber;
        });

    db1.insert(
        "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','Hallo')",
        [](void) -> void {
            VLOG(0) << "OnQuery";
        },
        [](const std::string& errorString, unsigned int errorNumber) -> void {
            VLOG(0) << "Error: " << errorString << " : " << errorNumber;
        });

    db1.query(
        "select * from snodec",
        [](const MYSQL_ROW row) -> void {
            if (row != nullptr) {
                VLOG(0) << "Row Result: " << row[0] << " : " << row[1];
            }
        },
        [](const std::string& errorString, unsigned int errorNumber) -> void {
            VLOG(0) << "Error: " << errorString << " : " << errorNumber;
        });

    database::mariadb::MariaDBClient db2(details);
    {
        db2.query(
            "select * from snodec",
            [](const MYSQL_ROW row) -> void {
                if (row != nullptr) {
                    VLOG(0) << "Row Result 1: " << row[0] << " : " << row[1];
                }
            },
            [](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "Error 1: " << errorString << " : " << errorNumber;
            });

        db2.query(
            "select * from snodec",
            [&db2](const MYSQL_ROW row) -> void {
                if (row != nullptr) {
                    VLOG(0) << "Row Result 2: " << row[0] << " : " << row[1];
                }

                db2.query(
                    "select * from snodec",
                    [&db2](const MYSQL_ROW row) -> void {
                        if (row != nullptr) {
                            VLOG(0) << "Row Result 3: " << row[0] << " : " << row[1];
                        }

                        core::timer::Timer dbTimer1 = core::timer::Timer::intervalTimer(
                            [&db2](const void* arg, const std::function<void()>& stop) -> void {
                                static int i = 0;
                                std::cout << static_cast<const char*>(arg) << " " << i++ << std::endl;

                                db2.query(
                                    "select * from snodec",
                                    [](const MYSQL_ROW row) -> void {
                                        if (row != nullptr) {
                                            VLOG(0) << "Row Result 4: " << row[0] << " : " << row[1];
                                        }
                                    },
                                    [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                        VLOG(0) << "Error 4: " << errorString << " : " << errorNumber;
                                        stop();
                                    });
                            },
                            2,
                            "Tick 2");

                        core::timer::Timer dbTimer2 = core::timer::Timer::intervalTimer(
                            [&db2](const void* arg, const std::function<void()>& stop) -> void {
                                static int i = 0;
                                std::cout << static_cast<const char*>(arg) << " " << i++ << std::endl;

                                db2.query(
                                    "select * from snodec",
                                    [](const MYSQL_ROW row) -> void {
                                        if (row != nullptr) {
                                            VLOG(0) << "Row Result 5: " << row[0] << " : " << row[1];
                                        }
                                    },
                                    [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                        VLOG(0) << "Error 5: " << errorString << " : " << errorNumber;
                                        stop();
                                    });
                            },
                            0.7,
                            "Tick 0.7");
                    },
                    [](const std::string& errorString, unsigned int errorNumber) -> void {
                        VLOG(0) << "Error 3: " << errorString << " : " << errorNumber;
                    });
            },
            [](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "Error 2: " << errorString << " : " << errorNumber;
            });

        core::timer::Timer dbTimer = core::timer::Timer::intervalTimer(
            [&db2](const void* arg, const std::function<void()>& stop) -> void {
                static int i = 0;
                std::cout << static_cast<const char*>(arg) << " " << i++ << std::endl;

                if (i >= 60000) {
                    VLOG(0) << "Stop Stop";
                    stop();
                }
                int j = i;
                db2.insert(
                    "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','Hallo')",
                    [j](void) -> void {
                        VLOG(0) << "OnQuery 6: " << j;
                    },
                    [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                        VLOG(0) << "Error 6: " << errorString << " : " << errorNumber;
                        stop();
                    });
            },
            1,
            "Tick 1");
    }

    return core::SNodeC::start();
}
