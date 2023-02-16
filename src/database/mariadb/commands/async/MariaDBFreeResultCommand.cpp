/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "database/mariadb/commands/async/MariaDBFreeResultCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands::async {

    MariaDBFreeResultCommand::MariaDBFreeResultCommand(MYSQL_RES*& result,
                                                       const std::function<void(void)>& onFreeResult,
                                                       const std::function<void(const std::string&, unsigned int)>& onError)
        : MariaDBCommandASync("FreeResult", onError)
        , result(result)
        , onFreeResult(onFreeResult) {
    }

    MariaDBFreeResultCommand::~MariaDBFreeResultCommand() {
        if (result != nullptr) {
            mysql_free_result(result);
        }
    }

    int MariaDBFreeResultCommand::commandStart() {
        int ret = 0;

        if (result != nullptr) {
            ret = mysql_free_result_start(result);
        }

        return ret;
    }

    int MariaDBFreeResultCommand::commandContinue(int status) {
        return mysql_free_result_cont(result, status);
    }

    bool MariaDBFreeResultCommand::commandCompleted() {
        onFreeResult();

        result = nullptr;

        return true;
    }

    void MariaDBFreeResultCommand::commandError(const std::string& errorString, unsigned int errorNumber) {
        onError(errorString, errorNumber);
    }

} // namespace database::mariadb::commands::async
