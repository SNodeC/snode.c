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

#include "core/DescriptorEventReceiver.h"
#include "database/mariadb/MariaDBClient.h"
#include "database/mariadb/MariaDBCommand.h"
#include "database/mariadb/commands/MariaDBConnectCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <mysql.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    MariaDBConnection::MariaDBConnection(MariaDBClient* mariaDBClient, const MariaDBConnectionDetails& connectionDetails)
        : ReadEventReceiver("MariaDBConnection", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , WriteEventReceiver("MariaDBConnection", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , ExceptionalConditionEventReceiver("MariaDBConnection", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , mariaDBClient(mariaDBClient)
        , connectionDetails(connectionDetails)
        , mysql(mysql_init(nullptr)) {
        mysql_options(mysql, MYSQL_OPT_NONBLOCK, 0);

        commandQueue.push_back(new database::mariadb::commands::MariaDBConnectCommand(
            this,
            connectionDetails,
            [](void) -> void {
                VLOG(0) << "Connected";
            },
            [](const std::string& errorString) -> void {
                VLOG(0) << "Connect error: " << errorString;
            }));
    }

    MariaDBConnection::~MariaDBConnection() {
        mysql_close(mysql);
        mysql_library_end();

        for (MariaDBCommand* mariaDBCommand : commandQueue) {
            delete mariaDBCommand;
        }

        if (mariaDBClient != nullptr) {
            mariaDBClient->connectionVanished();
        }
    }

    class MariaDBCommandExecuteReceiver : public core::EventReceiver {
    public:
        MariaDBCommandExecuteReceiver(const std::string& name, MariaDBConnection* mariaDBConnection, MariaDBCommand* mariaDBCommand)
            : core::EventReceiver(name)
            , mariaDBConnection(mariaDBConnection)
            , mariaDBCommand(mariaDBCommand) {
        }

        using core::EventReceiver::publish;

        static void publish(MariaDBConnection* mariaDBConnection, MariaDBCommand* mariaDBCommand) {
            (new MariaDBCommandExecuteReceiver("MariaDBCommandExecuteReceiver", mariaDBConnection, mariaDBCommand))->publish();
        }

    private:
        void dispatch([[maybe_unused]] const utils::Timeval& currentTime) override {
            mariaDBConnection->executeReal(mariaDBCommand);

            delete this;
        }

        MariaDBConnection* mariaDBConnection = nullptr;
        MariaDBCommand* mariaDBCommand = nullptr;
    };

    void MariaDBConnection::execute(MariaDBCommand* mariaDBCommand) {
        MariaDBCommandExecuteReceiver::publish(this, mariaDBCommand);
    }

    void MariaDBConnection::executeReal(MariaDBCommand* mariaDBCommand) {
        commandQueue.push_back(mariaDBCommand);

        if (currentCommand == nullptr) {
            commandExecute();
        }
    }

    void MariaDBConnection::executeAsNext(MariaDBCommand* mariaDBCommand) {
        commandCompleted();

        commandQueue.push_front(mariaDBCommand);

        if (currentCommand == nullptr) {
            commandExecute();
        }
    }

    void MariaDBConnection::commandExecute() {
        if (!commandQueue.empty()) {
            currentCommand = commandQueue.front();

            int status = currentCommand->start(mysql);

            checkStatus(status);
        } else {
            currentCommand = nullptr;

            if (mariaDBClient == nullptr) {
                ReadEventReceiver::disable();
                WriteEventReceiver::disable();
                ExceptionalConditionEventReceiver::disable();
            }
        }
    }

    void MariaDBConnection::commandContinue(int status) {
        if (currentCommand != nullptr) {
            int currentStatus = currentCommand->cont(mysql, status);

            checkStatus(currentStatus);
        } else if ((status & MYSQL_WAIT_READ) != 0) {
            VLOG(0) << "ReadEvent but no command running - must be a disconnect";

            ReadEventReceiver::disable();
            WriteEventReceiver::disable();
            ExceptionalConditionEventReceiver::disable();
        }
    }

    void MariaDBConnection::commandCompleted() {
        commandQueue.pop_front();

        if (currentCommand != nullptr) {
            delete currentCommand;
        }
    }

    void MariaDBConnection::unmanaged() {
        mariaDBClient = nullptr;
    }

    void MariaDBConnection::setFd(int status) {
        if ((status & MYSQL_WAIT_READ) != 0 || (status & MYSQL_WAIT_WRITE) != 0 || (status & MYSQL_WAIT_EXCEPT) != 0 ||
            (status == 0 && !currentCommand->error())) {
            int fd = mysql_get_socket(mysql);

            ReadEventReceiver::enable(fd);
            WriteEventReceiver::enable(fd);
            ExceptionalConditionEventReceiver::enable(fd);

            WriteEventReceiver::suspend();
            ExceptionalConditionEventReceiver::suspend();

            connected = true;
        } else {
            VLOG(0) << "Mysql setFd-Error: " << mysql_error(mysql) << ", " << mysql_errno(mysql);
        }
    }

    void MariaDBConnection::checkStatus(int status) {
        if (status == 0) {
            if (connected) {
                if (!currentCommand->error()) {
                    currentCommand->commandCompleted();
                } else {
                    currentCommand->commandError(mysql_error(mysql), mysql_errno(mysql));
                }

                commandExecute();
            } else {
                delete this;
            }
        } else {
            if ((status & MYSQL_WAIT_WRITE) != 0) {
                if (WriteEventReceiver::isSuspended()) {
                    WriteEventReceiver::resume();
                }
            } else if (!WriteEventReceiver::isSuspended()) {
                WriteEventReceiver::suspend();
            }

            if ((status & MYSQL_WAIT_EXCEPT) != 0) {
                if (ExceptionalConditionEventReceiver::isSuspended()) {
                    ExceptionalConditionEventReceiver::resume();
                }
            } else if (!ExceptionalConditionEventReceiver::isSuspended()) {
                ExceptionalConditionEventReceiver::suspend();
            }

            if ((status & MYSQL_WAIT_TIMEOUT) != 0) {
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
        commandContinue(MYSQL_WAIT_READ);
    }

    void MariaDBConnection::writeEvent() {
        commandContinue(MYSQL_WAIT_WRITE);
    }

    void MariaDBConnection::outOfBandEvent() {
        commandContinue(MYSQL_WAIT_EXCEPT);
    }

    void MariaDBConnection::readTimeout() {
        commandContinue(MYSQL_WAIT_TIMEOUT);
    }

    void MariaDBConnection::writeTimeout() {
        commandContinue(MYSQL_WAIT_TIMEOUT);
    }

    void MariaDBConnection::outOfBandTimeout() {
        commandContinue(MYSQL_WAIT_TIMEOUT);
    }

    void MariaDBConnection::unobservedEvent() {
        delete this;
    }

} // namespace database::mariadb
