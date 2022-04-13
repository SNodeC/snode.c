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

#include "database/mariadb/commands/MariaDBQueryCommand.h"

#include "database/mariadb/MariaDBConnection.h"
#include "database/mariadb/commands/MariaDBFetchRowCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <mysql.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands {

    MariaDBQueryCommand::MariaDBQueryCommand(MariaDBConnection* mariaDBConnection,
                                             const std::string& sql,
                                             const std::function<void(void)>& onQuery,
                                             const std::function<void(const std::string&)>& onError)
        : MariaDBCommand(mariaDBConnection, "Query")
        , sql(sql)
        , onQuery(onQuery)
        , onError(onError) {
    }

    int MariaDBQueryCommand::start() {
        return mysql_real_query_start(&ret, mysql, sql.c_str(), sql.length());
    }

    int MariaDBQueryCommand::cont(int status) {
        return mysql_real_query_cont(&ret, mysql, status);
    }

    void MariaDBQueryCommand::commandCompleted() {
        onQuery();

        MYSQL_RES* result = mysql_use_result(mysql);

        if (ret == 0 && result != nullptr) {
            mariaDBConnection->executeAsNext(
                new database::mariadb::commands::MariaDBFetchRowCommand(mariaDBConnection, result, [](MYSQL_ROW row) -> void {
                    VLOG(0) << "Row Result: " << row[0] << " : " << row[1];
                }));
        } else {
            mariaDBConnection->commandCompleted();
        }
    }

    void MariaDBQueryCommand::commandError(const std::string& errorString, [[maybe_unused]] unsigned int errorNumber) {
        onError(errorString);
    }

    bool MariaDBQueryCommand::error() {
        return ret != 0;
    }

} // namespace database::mariadb::commands
