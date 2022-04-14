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

#include <mysql.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands {

    MariaDBFetchRowCommand::MariaDBFetchRowCommand(MariaDBConnection* mariaDBConnection,
                                                   MYSQL_RES* result,
                                                   const std::function<void(const MYSQL_ROW)>& onRowResult,
                                                   const std::function<void(const std::string&, unsigned int)>& onError)
        : MariaDBCommand(mariaDBConnection, "FetschRow", onError)
        , result(result)
        , onRowResult(onRowResult) {
    }

    int MariaDBFetchRowCommand::commandStart() {
        int ret = 0;

        if (result != nullptr) {
            ret = mysql_fetch_row_start(&row, result);
        }

        return ret;
    }

    int MariaDBFetchRowCommand::commandContinue(int status) {
        int ret = mysql_fetch_row_cont(&row, result, status);

        return ret;
    }

    void MariaDBFetchRowCommand::commandCompleted() {
        onRowResult(row);

        if (row == nullptr) {
            mysql_free_result(result);
            mariaDBConnection->commandCompleted();
        }
    }

    void MariaDBFetchRowCommand::commandError(const std::string& errorString, unsigned int errorNumber) {
        onError(errorString, errorNumber);
    }

    bool MariaDBFetchRowCommand::error() {
        return result == nullptr;
    }

} // namespace database::mariadb::commands
