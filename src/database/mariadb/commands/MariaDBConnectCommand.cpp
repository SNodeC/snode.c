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

#include "database/mariadb/commands/MariaDBConnectCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands {

    MariaDBConnectCommand::MariaDBConnectCommand(const MariaDBConnectionDetails& details,
                                                 const std::function<void()>& onConnect,
                                                 const std::function<void(const std::string&)>& onError)
        : MariaDBCommand::MariaDBCommand()
        , details(details)
        , onConnect(onConnect)
        , onError(onError) {
    }

    int MariaDBConnectCommand::start(MYSQL* mysql) {
        return mysql_real_connect_start(&ret,
                                        mysql,
                                        details.hostname.c_str(),
                                        details.username.c_str(),
                                        details.password.c_str(),
                                        details.database.c_str(),
                                        details.port,
                                        details.socket.c_str(),
                                        details.flags);
    }

    int MariaDBConnectCommand::cont(MYSQL* mysql, int status) {
        return mysql_real_connect_cont(&ret, mysql, status);
    }

    void MariaDBConnectCommand::commandCompleted() {
        onConnect();
    }

    void MariaDBConnectCommand::commandError(const std::string& errorString) {
        onError(errorString);
    }

    bool MariaDBConnectCommand::error() {
        return ret == nullptr;
    }

} // namespace database::mariadb::commands
