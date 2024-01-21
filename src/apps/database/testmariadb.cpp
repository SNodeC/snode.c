/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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
#include "database/mariadb/MariaDBCommandSequence.h"
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdlib>
#include <functional>
#include <mysql.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--db-host", "Hostname of IP-Address of Server", "[hostname|IP-address]", "localhost", true);

    core::SNodeC::init(argc, argv);

    const database::mariadb::MariaDBConnectionDetails details = {
        .hostname = utils::Config::get_string_option_value("--db-host"),
        .username = "snodec",
        .password = "pentium5",
        .database = "snodec",
        .port = 3306,
        .socket = "/run/mysqld/mysqld.sock",
        .flags = 0,
    };

    // CREATE DATABASE snodec;
    // CREATE TABLE snodec (username text NOT NULL, password text NOT NULL ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    // CREATE USER 'snodec'@localhost IDENTIFIED BY 'pentium5';
    // GRANT ALL PRIVILEGES ON *.* TO 'snodec'@localhost;
    // GRANT ALL PRIVILEGES ON snodec.snodec TO 'snodec'@localhost;
    // FLUSH PRIVILEGES;

    database::mariadb::MariaDBClient db1(details);

    int r = 0;

    db1.exec(
           "DELETE FROM `snodec`",
           [&db1](void) -> void {
               VLOG(0) << "********** OnQuery 0;";
               db1.affectedRows(
                   [](my_ulonglong affectedRows) -> void {
                       VLOG(0) << "********** AffectedRows 1: " << affectedRows;
                   },
                   [](const std::string& errorString, unsigned int errorNumber) -> void {
                       VLOG(0) << "Error 1: " << errorString << " : " << errorNumber;
                   });
           },
           [](const std::string& errorString, unsigned int errorNumber) -> void {
               VLOG(0) << "********** Error 0: " << errorString << " : " << errorNumber;
           })

        .exec(
            "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','Hallo')",
            [&db1](void) -> void {
                VLOG(0) << "********** OnQuery 1: ";
                db1.affectedRows(
                    [](my_ulonglong affectedRows) -> void {
                        VLOG(0) << "********** AffectedRows 2: " << affectedRows;
                    },
                    [](const std::string& errorString, unsigned int errorNumber) -> void {
                        VLOG(0) << "********** Error 2: " << errorString << " : " << errorNumber;
                    });
            },
            [](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "********** Error 1: " << errorString << " : " << errorNumber;
            })
        .query(
            "SELECT * FROM snodec",
            [&r](const MYSQL_ROW row) -> void {
                if (row != nullptr) {
                    VLOG(0) << "********** Row Result 2: " << row[0] << " : " << row[1];
                    r++;
                } else {
                    VLOG(0) << "********** Row Result 2: " << r;
                }
            },
            [](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "********** Error 2: " << errorString << " : " << errorNumber;
            })
        .query(
            "SELECT * FROM snodec",
            [&r](const MYSQL_ROW row) -> void {
                if (row != nullptr) {
                    VLOG(0) << "********** Row Result 2: " << row[0] << " : " << row[1];
                    r++;
                } else {
                    VLOG(0) << "********** Row Result 2: " << r;
                }
            },
            [](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "********** Error 2: " << errorString << " : " << errorNumber;
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
                                    [&db2, &r1](const std::function<void()>& stop) -> void {
                                        static int i = 0;
                                        VLOG(0) << "Tick 2: " << i++;

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
                                    2);

                                core::timer::Timer dbTimer2 = core::timer::Timer::intervalTimer(
                                    [&db2, &r2](const std::function<void()>& stop) -> void {
                                        static int i = 0;
                                        VLOG(0) << "Tick 0.7: " << i++;

                                        r2 = 0;
                                        db2.query(
                                               "SELECT * FROM snodec",
                                               [&db2, &r2](const MYSQL_ROW row) -> void {
                                                   if (row != nullptr) {
                                                       VLOG(0) << "Row Result 7: " << row[0] << " : " << row[1];
                                                       r2++;
                                                   } else {
                                                       VLOG(0) << "Row Result 7: " << r2;
                                                       db2.fieldCount(
                                                           [](unsigned int fieldCount) -> void {
                                                               VLOG(0) << "************ FieldCount ************ = " << fieldCount;
                                                           },
                                                           [](const std::string& errorString, unsigned int errorNumber) -> void {
                                                               VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
                                                           });
                                                   }
                                               },
                                               [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                                                   VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
                                                   stop();
                                               })
                                            .fieldCount(
                                                [](unsigned int fieldCount) -> void {
                                                    VLOG(0) << "************ FieldCount ************ = " << fieldCount;
                                                },
                                                [](const std::string& errorString, unsigned int errorNumber) -> void {
                                                    VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
                                                });
                                    },
                                    0.7);
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
            [&db2](const std::function<void()>& stop) -> void {
                static int i = 0;
                VLOG(0) << "Tick 0.1: " << i++;

                if (i >= 60000) {
                    VLOG(0) << "Stop Stop";
                    stop();
                }

                int j = i;
                db2.startTransactions(
                       [](void) -> void {
                           VLOG(0) << "Transactions activated 10:";
                       },
                       [](const std::string& errorString, unsigned int errorNumber) -> void {
                           VLOG(0) << "Error 8: " << errorString << " : " << errorNumber;
                       })
                    .exec(
                        "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','Hallo')",
                        [&db2, j](void) -> void {
                            VLOG(0) << "Inserted 10: " << j;
                            db2.affectedRows(
                                [](my_ulonglong affectedRows) -> void {
                                    VLOG(0) << "AffectedRows 11: " << affectedRows;
                                },
                                [](const std::string& errorString, unsigned int errorNumber) -> void {
                                    VLOG(0) << "Error 11: " << errorString << " : " << errorNumber;
                                });
                        },
                        [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                            VLOG(0) << "Error 10: " << errorString << " : " << errorNumber;
                            stop();
                        })
                    .rollback(
                        [](void) -> void {
                            VLOG(0) << "Rollback success 11";
                        },
                        [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                            VLOG(0) << "Error 12: " << errorString << " : " << errorNumber;
                            stop();
                        })
                    .exec(
                        "INSERT INTO `snodec`(`username`, `password`) VALUES ('Annett','Hallo')",
                        [&db2, j](void) -> void {
                            VLOG(0) << "Inserted 13: " << j;
                            db2.affectedRows(
                                [](my_ulonglong affectedRows) -> void {
                                    VLOG(0) << "AffectedRows 14: " << affectedRows;
                                },
                                [](const std::string& errorString, unsigned int errorNumber) -> void {
                                    VLOG(0) << "Error 14: " << errorString << " : " << errorNumber;
                                });
                        },
                        [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                            VLOG(0) << "Error 13: " << errorString << " : " << errorNumber;
                            stop();
                        })
                    .commit(
                        [](void) -> void {
                            VLOG(0) << "Commit success 15";
                        },
                        [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                            VLOG(0) << "Error 15: " << errorString << " : " << errorNumber;
                            stop();
                        })
                    .query(
                        "SELECT COUNT(*) FROM snodec",
                        [&db2, j, stop](const MYSQL_ROW row) -> void {
                            if (row != nullptr) {
                                VLOG(0) << "Row Result count(*) 16: " << row[0];
                                if (std::atoi(row[0]) != j + 1) {                                                   // NOLINT
                                    VLOG(0) << "Wrong number of rows 16: " << std::atoi(row[0]) << " != " << j + 1; // NOLINT
                                    //                                    exit(1);
                                }
                            } else {
                                VLOG(0) << "Row Result count(*) 16: no result:";
                                db2.fieldCount(
                                    [](unsigned int fieldCount) -> void {
                                        VLOG(0) << "************ FieldCount ************ = " << fieldCount;
                                    },
                                    [](const std::string& errorString, unsigned int errorNumber) -> void {
                                        VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
                                    });
                            }
                        },
                        [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                            VLOG(0) << "Error 16: " << errorString << " : " << errorNumber;
                            stop();
                        })
                    .endTransactions(
                        [](void) -> void {
                            VLOG(0) << "Transactions deactivated 17";
                        },
                        [stop](const std::string& errorString, unsigned int errorNumber) -> void {
                            VLOG(0) << "Error 17: " << errorString << " : " << errorNumber;
                            stop();
                        });
            },
            0.1);
    }

    return core::SNodeC::start();
}
