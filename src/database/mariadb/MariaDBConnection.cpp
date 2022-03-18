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

#include "database/mariadb/MariaDBConnection.h"

#include "database/mariadb/MariaDBClient.h"
#include "database/mariadb/commands/MariaDBConnectCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <mysql.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    MariaDBConnection::MariaDBConnection(MariaDBClient* mariaDBClient, const MariaDBConnectionDetails& connectionDetails)
        : ReadEventReceiver("MariaDBConnection", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , WriteEventReceiver("MariaDBConnection", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , ExceptionalConditionEventReceiver("MariaDBConnection", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , mariaDBClient(mariaDBClient)
        , mysql(mysql_init(nullptr))
        , managed(true)
        , connectionDetails(connectionDetails) {
        mysql_options(mysql, MYSQL_OPT_NONBLOCK, 0);
    }

    MariaDBConnection::~MariaDBConnection() {
        mysql_close(mysql);
        mysql_library_end();
        mariaDBClient->connectionVanished();
    }

    void MariaDBConnection::executeCommand(MariaDBCommand* mariaDBCommand) {
        currentCommand = mariaDBCommand;

        if (currentCommand != nullptr) {
            int currentStatus = 0;
            if (!connected) {
                MariaDBCommand* connectCommand = new database::mariadb::command::MariaDBConnectCommand(
                    connectionDetails,
                    [mariaDBCommand, this](void) -> void {
                        connected = true;
                        currentCommand = mariaDBCommand;

                        int currentStatus = currentCommand->start(mysql);
                        checkStatus(currentStatus);
                    },
                    [mariaDBCommand](const std::string& errorString) -> void {
                        VLOG(0) << "Connect error: " << errorString;
                        delete mariaDBCommand;
                    });

                currentCommand = connectCommand;
                currentStatus = connectCommand->start(mysql);

                if (currentStatus & MYSQL_WAIT_READ || currentStatus & MYSQL_WAIT_WRITE || currentStatus & MYSQL_WAIT_EXCEPT ||
                    (currentStatus == 0 && !currentCommand->error())) {
                    int fd = mysql_get_socket(mysql);

                    ReadEventReceiver::enable(fd);
                    WriteEventReceiver::enable(fd);
                    ExceptionalConditionEventReceiver::enable(fd);

                    ReadEventReceiver::suspend();
                    WriteEventReceiver::suspend();
                    ExceptionalConditionEventReceiver::suspend();
                }

            } else {
                currentStatus = currentCommand->start(mysql);
            }

            checkStatus(currentStatus);
        }
    }

    void MariaDBConnection::continueCommand(int status) {
        if (currentCommand != nullptr) {
            int currentStatus = currentCommand->cont(mysql, status);

            checkStatus(currentStatus);
        }
    }

    void MariaDBConnection::unmanaged() {
        managed = false;
    }

    void MariaDBConnection::checkStatus(int status) {
        if (status == 0) {
            if (ReadEventReceiver::isEnabled() && !ReadEventReceiver::isSuspended()) {
                ReadEventReceiver::suspend();
            }
            if (WriteEventReceiver::isEnabled() && !WriteEventReceiver::isSuspended()) {
                WriteEventReceiver::suspend();
            }
            if (ExceptionalConditionEventReceiver::isEnabled() && !ExceptionalConditionEventReceiver::isSuspended()) {
                ExceptionalConditionEventReceiver::suspend();
            }

            MariaDBCommand* oldCommand = currentCommand;
            currentCommand = nullptr;

            if (!oldCommand->error()) {
                oldCommand->commandCompleted();

            } else {
                oldCommand->commandError(mysql_error(mysql));
            }
            delete oldCommand;

            if (!managed) {
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                ExceptionalConditionEventReceiver::disable();
            }
        } else {
            if (status & MYSQL_WAIT_READ) {
                if (ReadEventReceiver::isSuspended()) {
                    ReadEventReceiver::resume();
                }
            } else if (!ReadEventReceiver::isSuspended()) {
                ReadEventReceiver::suspend();
            }
            if (status & MYSQL_WAIT_WRITE) {
                if (WriteEventReceiver::isSuspended()) {
                    WriteEventReceiver::resume();
                }
            } else if (!WriteEventReceiver::isSuspended()) {
                WriteEventReceiver::suspend();
            }
            if (status & MYSQL_WAIT_EXCEPT) {
                if (ExceptionalConditionEventReceiver::isSuspended()) {
                    ExceptionalConditionEventReceiver::resume();
                }
            } else if (!ExceptionalConditionEventReceiver::isSuspended()) {
                ExceptionalConditionEventReceiver::suspend();
            }
            if (status & MYSQL_WAIT_TIMEOUT) {
                ReadEventReceiver::setTimeout(mysql_get_timeout_value(mysql));
                WriteEventReceiver::setTimeout(mysql_get_timeout_value(mysql));
                ExceptionalConditionEventReceiver::setTimeout(mysql_get_timeout_value(mysql));
            } else {
                ReadEventReceiver::setTimeout(core::DescriptorEventReceiver::TIMEOUT::DEFAULT);
                WriteEventReceiver::setTimeout(core::DescriptorEventReceiver::TIMEOUT::DEFAULT);
                ExceptionalConditionEventReceiver::setTimeout(core::DescriptorEventReceiver::TIMEOUT::DEFAULT);
            }
        }
    }

    void MariaDBConnection::readEvent() {
        continueCommand(MYSQL_WAIT_READ);
    }

    void MariaDBConnection::writeEvent() {
        continueCommand(MYSQL_WAIT_WRITE);
    }

    void MariaDBConnection::outOfBandEvent() {
        continueCommand(MYSQL_WAIT_EXCEPT);
    }

    void MariaDBConnection::readTimeout() {
        continueCommand(MYSQL_WAIT_TIMEOUT);
    }

    void MariaDBConnection::writeTimeout() {
        continueCommand(MYSQL_WAIT_TIMEOUT);
    }

    void MariaDBConnection::outOfBandTimeout() {
        continueCommand(MYSQL_WAIT_TIMEOUT);
    }

    void MariaDBConnection::unobservedEvent() {
        delete this;
    }

} // namespace database::mariadb
