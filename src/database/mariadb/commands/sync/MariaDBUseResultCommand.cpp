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

#include "database/mariadb/commands/sync/MariaDBUseResultCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands::sync {

    MariaDBUseResultCommand::MariaDBUseResultCommand(const std::function<void(MYSQL_RES*)>& onUseResult,
                                                     const std::function<void(const std::string&, unsigned int)>& onError)
        : MariaDBCommandSync("UseResult", onError)
        , onUseResult(onUseResult) {
    }

    int MariaDBUseResultCommand::commandStart() {
        result = mysql_use_result(mysql);

        return 0;
    }

    bool MariaDBUseResultCommand::commandCompleted() {
        onUseResult(result);

        return true;
    }

    void MariaDBUseResultCommand::commandError(const std::string& errorString, unsigned int errorNumber) {
        onError(errorString, errorNumber);
    }

} // namespace database::mariadb::commands::sync
