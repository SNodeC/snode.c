/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "database/mariadb/commands/async/MariaDBConnectCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands::async {

    MariaDBConnectCommand::MariaDBConnectCommand(const MariaDBConnectionDetails& details,
                                                 const std::function<void(void)>& onConnecting,
                                                 const std::function<void(void)>& onConnect,
                                                 const std::function<void(const std::string&, unsigned int)>& onError)
        : MariaDBCommandASync("Connect", onError)
        , details(details)
        , onConnecting(onConnecting)
        , onConnect(onConnect) {
    }

    int MariaDBConnectCommand::commandStart() {
        const int status = mysql_real_connect_start(&ret,
                                                    mysql,
                                                    details.hostname.c_str(),
                                                    details.username.c_str(),
                                                    details.password.c_str(),
                                                    details.database.c_str(),
                                                    details.port,
                                                    details.socket.c_str(),
                                                    details.flags);

        onConnecting();

        return status;
    }

    int MariaDBConnectCommand::commandContinue(int status) {
        return mysql_real_connect_cont(&ret, mysql, status);
    }

    bool MariaDBConnectCommand::commandCompleted() {
        onConnect();

        return true;
    }

    void MariaDBConnectCommand::commandError(const std::string& errorString, unsigned int errorNumber) {
        onError(errorString, errorNumber);
    }

    std::string MariaDBConnectCommand::commandInfo() {
        return commandName() + ": " + details.hostname + ":" + std::to_string(details.port) + " | " + details.socket;
    }

} // namespace database::mariadb::commands::async
