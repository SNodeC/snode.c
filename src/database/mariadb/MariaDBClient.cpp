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

#include "database/mariadb/MariaDBClient.h"

#include "database/mariadb/MariaDBConnection.h"
#include "database/mariadb/commands/MariaDBQueryCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace std;

namespace database::mariadb {

    MariaDBClient::MariaDBClient(const MariaDBConnectionDetails& details)
        : mariaDBConnection(new MariaDBConnection(this, details)) {
    }

    MariaDBClient::~MariaDBClient() {
        if (mariaDBConnection != nullptr) {
            mariaDBConnection->unmanaged();
        }
    }

    void
    MariaDBClient::query(const std::string& sql, const function<void()>& onQuery, const std::function<void(const std::string&)> onError) {
        execute(new database::mariadb::commands::MariaDBQueryCommand(mariaDBConnection, sql, onQuery, onError));
    }

    void MariaDBClient::execute(MariaDBCommand* mariaDBCommand) {
        if (mariaDBConnection != nullptr) {
            mariaDBConnection->execute(mariaDBCommand);
        } else {
            mariaDBCommand->commandError("No connection to database", 0);
            delete mariaDBCommand;
        }
    }

    void MariaDBClient::disconnect([[maybe_unused]] const function<void()>& onDisconnect) {
        VLOG(0) << "Disconnect not implemented yet.";
    }

    void MariaDBClient::connectionVanished() {
        mariaDBConnection = nullptr;
    }

} // namespace database::mariadb
