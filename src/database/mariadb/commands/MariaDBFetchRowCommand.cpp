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

#include "database/mariadb/commands/MariaDBFetchRowCommand.h"

#include "database/mariadb/MariaDBConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands {

    MariaDBFetchRowCommand::MariaDBFetchRowCommand(MariaDBConnection* mariaDBConnection,
                                                   const std::function<void(MYSQL_ROW row)>& onRowResult)
        : MariaDBCommand(mariaDBConnection)
        , onRowResult(onRowResult) {
    }

    MariaDBFetchRowCommand::~MariaDBFetchRowCommand() {
        mysql_free_result(result);
    }

    int MariaDBFetchRowCommand::start(MYSQL* mysql) {
        row = nullptr;

        int ret = 0;

        if (result == nullptr) {
            result = mysql_use_result(mysql);
        }

        if (result != nullptr) {
            ret = mysql_fetch_row_start(&row, result);
        }

        return ret;
    }

    int MariaDBFetchRowCommand::cont([[maybe_unused]] MYSQL* mysql, int status) {
        return mysql_fetch_row_cont(&row, result, status);
    }

    void MariaDBFetchRowCommand::commandCompleted() {
        if (row) {
            onRowResult(row);
        } else {
            VLOG(0) << "No result: " << result;
            mariaDBConnection->commandCompleted();
        }
    }

    void MariaDBFetchRowCommand::commandError(const std::string& errorString, unsigned int errorNumber) {
        VLOG(0) << "FetchRowError: " << errorString << ", errno = " << errorNumber;
        mariaDBConnection->commandCompleted();
    }

    bool MariaDBFetchRowCommand::error() {
        return false;
    }

} // namespace database::mariadb::commands
