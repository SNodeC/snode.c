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

#include "database/mariadb/commands/MariaDBAutoCommitCommand.h"

#include "database/mariadb/MariaDBConnection.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands {

    MariaDBAutoCommitCommand::MariaDBAutoCommitCommand(MariaDBConnection* mariaDBConnection,
                                                       my_bool autoCommit,
                                                       const std::function<void()>& onSet,
                                                       const std::function<void(const std::string&, unsigned int)>& onError)
        : MariaDBCommand(mariaDBConnection, "AutoCommit", onError)
        , autoCommit(autoCommit)
        , onSet(onSet) {
    }

    int MariaDBAutoCommitCommand::commandStart() {
        return mysql_autocommit_start(&ret, mysql, autoCommit);
    }

    int MariaDBAutoCommitCommand::commandContinue(int status) {
        return mysql_autocommit_cont(&ret, mysql, status);
    }

    void MariaDBAutoCommitCommand::commandCompleted() {
        onSet();
        mariaDBConnection->commandCompleted();
    }

    void MariaDBAutoCommitCommand::commandError(const std::string& errorString, unsigned int errorNumber) {
        onError(errorString, errorNumber);
    }

    std::string MariaDBAutoCommitCommand::commandInfo() {
        return commandName() + ": AutoCommit = " + std::to_string(autoCommit);
    }

} // namespace database::mariadb::commands
