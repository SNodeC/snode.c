/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "database/mariadb/commands/async/MariaDBRollbackCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands::async {

    MariaDBRollbackCommand::MariaDBRollbackCommand(const std::function<void()>& onRollback,
                                                   const std::function<void(const std::string&, unsigned int)>& onError)
        : MariaDBCommandASync("Rollback", onError)
        , onRollback(onRollback) {
    }

    int MariaDBRollbackCommand::commandStart() {
        return mysql_rollback_start(&ret, mysql);
    }

    int MariaDBRollbackCommand::commandContinue(int status) {
        return mysql_rollback_cont(&ret, mysql, status);
    }

    bool MariaDBRollbackCommand::commandCompleted() {
        onRollback();

        return true;
    }

    void MariaDBRollbackCommand::commandError(const std::string& errorString, unsigned int errorNumber) {
        onError(errorString, errorNumber);
    }

} // namespace database::mariadb::commands::async
