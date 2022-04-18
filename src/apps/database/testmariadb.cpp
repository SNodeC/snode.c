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

    // CREATE USER 'snodec'@localhost IDENTIFIED BY 'pentium5'
    // GRANT ALL PRIVILEGES ON *.* TO 'snodec'@localhost
    // GRANT ALL PRIVILEGES ON 'snodec'.'snodec' TO 'snodec'@localhost
    // CREATE DATABASE 'snodec';
    // CREATE TABLE 'snodec' ('username' text NOT NULL, 'password' text NOT NULL ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4

    database::mariadb::MariaDBClient db1(details);

    db1.query(
        "DELETE FROM `snodec`",
        []([[maybe_unused]] const MYSQL_ROW row) -> void {
            VLOG(0) << "OnQuery 0";
        },
        [](const std::string& errorString, unsigned int errorNumber) -> void {
            VLOG(0) << "Error 0: " << errorString << " : " << errorNumber;
        });

    db1.insert(
        "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','Hallo')",
        [](void) -> void {
            VLOG(0) << "OnQuery 1";
        },
        [](const std::string& errorString, unsigned int errorNumber) -> void {
            VLOG(0) << "Error 1: " << errorString << " : " << errorNumber;
        });

    int r = 0;
    db1.query(
        "SELECT * FROM snodec",
        [&](const MYSQL_ROW row) -> void {
            if (row != nullptr) {
                VLOG(0) << "Row Result 2: " << row[0] << " : " << row[1];
                r++;
            } else {
                VLOG(0) << "Row Result 2: " << r;
            }
        },
        [&](const std::string& errorString, unsigned int errorNumber) -> void {
            VLOG(0) << "Error 2: " << errorString << " : " << errorNumber;
        });

    database::mariadb::MariaDBClient db2(details);
    {
        db2.query(
            "SELECT * FROM snodec",
            [](const MYSQL_ROW row) -> void {
                if (row != nullptr) {
                    VLOG(0) << "Row Result 3: " << row[0] << " : " << row[1];
                } else {
                    VLOG(0) << "Row Result 3:";
                }
            },
            [](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "Error 3: " << errorString << " : " << errorNumber;
            });

        int r1 = 0;
        int r2 = 0;

        db2.query(
            "SELECT * FROM snodec",
            [&db2, &r1, &r2](const MYSQL_ROW row) -> void {
                if (row != nullptr) {
                    VLOG(0) << "Row Result 4: " << row[0] << " : " << row[1];
                } else {
                    VLOG(0) << "Row Result 4:";

                    db2.query(
                        "SELECT * FROM snodec",
                        [&db2, &r1, &r2](const MYSQL_ROW row) -> void {
                            if (row != nullptr) {
                                VLOG(0) << "Row Result 5: " << row[0] << " : " << row[1];
                            } else { // After all results have been fetched
                                VLOG(0) << "Row Result 5:";

                                core::timer::Timer dbTimer1 = core::timer::Timer::intervalTimer(
                                    [&db2, &r1](const void* arg, const std::function<void()>& stop) -> void {
                                        static int i = 0;
                                        VLOG(0) << static_cast<const char*>(arg) << " " << i++;

                                        r1 = 0;
                                        db2.query(
                                            "SELECT * FROM snodec",
                                            [&r1](const MYSQL_ROW row) -> void {
                                                if (row != nullptr) {
                                                    VLOG(0) << "Row Result 6: " << row[0] << " : " << row[1];
                                                    r1++;
                                                } else {
                                                    VLOG(0) << "Row Result 6: " << r1;
                                                }
                                            },
                                            [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                                VLOG(0) << "Error 6: " << errorString << " : " << errorNumber;
                                                stop();
                                            });
                                    },
                                    2,
                                    "Tick 2");

                                core::timer::Timer dbTimer2 = core::timer::Timer::intervalTimer(
                                    [&db2, &r2](const void* arg, const std::function<void()>& stop) -> void {
                                        static int i = 0;
                                        VLOG(0) << static_cast<const char*>(arg) << " " << i++;

                                        r2 = 0;
                                        db2.query(
                                            "SELECT * FROM snodec",
                                            [&r2](const MYSQL_ROW row) -> void {
                                                if (row != nullptr) {
                                                    VLOG(0) << "Row Result 7: " << row[0] << " : " << row[1];
                                                    r2++;
                                                } else {
                                                    VLOG(0) << "Row Result 7: " << r2;
                                                }
                                            },
                                            [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                                VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
                                                stop();
                                            });
                                    },
                                    0.7,
                                    "Tick 0.7");
                            }
                        },
                        [](const std::string& errorString, unsigned int errorNumber) -> void {
                            VLOG(0) << "Error 5: " << errorString << " : " << errorNumber;
                        });
                }
            },
            [](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "Error 4: " << errorString << " : " << errorNumber;
            });

        core::timer::Timer dbTimer = core::timer::Timer::intervalTimer(
            [&db2](const void* arg, const std::function<void()>& stop) -> void {
                static int i = 0;
                VLOG(0) << static_cast<const char*>(arg) << " " << i++;

                if (i >= 60000) {
                    VLOG(0) << "Stop Stop";
                    stop();
                }

                int j = i;
                db2.startTransactions(
                    //                db2.insert(
                    //                    "START TRANSACTION",
                    [&db2, j, stop](void) -> void {
                        VLOG(0) << "Transactions activated";

                        db2.insert(
                            "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','Hallo')",
                            [j](void) -> void {
                                VLOG(0) << "Inserted 8: " << j;
                            },
                            [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                VLOG(0) << "Error 8: " << errorString << " : " << errorNumber;
                                stop();
                            });

                        db2.rollback(
                            [](void) -> void {
                                VLOG(0) << "Rollback success 9";
                            },
                            [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                VLOG(0) << "Error 9: " << errorString << " : " << errorNumber;
                                stop();
                            });

                        db2.insert(
                            "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','Hallo')",
                            [j](void) -> void {
                                VLOG(0) << "Inserted 10: " << j;
                            },
                            [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                VLOG(0) << "Error 10: " << errorString << " : " << errorNumber;
                                stop();
                            });

                        db2.commit(
                            [](void) -> void {
                                VLOG(0) << "Commit success 11";
                            },
                            [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                VLOG(0) << "Error 11: " << errorString << " : " << errorNumber;
                                stop();
                            });

                        db2.endTransactions(
                            [](void) -> void {
                                VLOG(0) << "Transactions deactivated 12";
                            },
                            [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                VLOG(0) << "Error 12: " << errorString << " : " << errorNumber;
                                stop();
                            });

                        db2.query(
                            "SELECT COUNT(*) FROM snodec",
                            [j, stop](const MYSQL_ROW row) -> void {
                                if (row != nullptr) {
                                    VLOG(0) << "Row Result count(*) 13: " << row[0];
                                    if (std::atoi(row[0]) != j + 1) {
                                        VLOG(0) << "Wrong number of rows 13: " << std::atoi(row[0]) << " != " << j + 1;
                                        //                                        stop();
                                    }
                                } else {
                                    VLOG(0) << "Row Result count(*): no result:";
                                }
                            },
                            [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                VLOG(0) << "Error 13: " << errorString << " : " << errorNumber;
                                stop();
                            });
                    },
                    [&db2, stop](const std::string& errorString, unsigned int errorNumber) -> void {
                        VLOG(0) << "Error 14: " << errorString << " : " << errorNumber;

                        db2.endTransactions(
                            [](void) -> void {
                                VLOG(0) << "Transactions deactivated 15:";
                            },
                            [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                VLOG(0) << "Error 15: " << errorString << " : " << errorNumber;
                                stop();
                            });
                    });
            },
            0.05,
            "Tick 0.05");
    }

    return core::SNodeC::start();
}
