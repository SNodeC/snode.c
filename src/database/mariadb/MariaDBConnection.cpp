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
        : ReadEventReceiver("MariaDBConnectionRead", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , WriteEventReceiver("MariaDBConnectionWrite", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , ExceptionalConditionEventReceiver("MariaDBConnectionExceptional", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , mariaDBClient(mariaDBClient)
        , commandStartEvent("MariaDBCommandStartEvent", this)
        , connectionDetails(connectionDetails)
        , mysql(mysql_init(nullptr)) {
        mysql_options(mysql, MYSQL_OPT_NONBLOCK, 0);

        commandQueue.push_back(new database::mariadb::commands::MariaDBConnectCommand(
            this,
            connectionDetails,
            [](void) -> void {
                VLOG(0) << "Connected";
            },
            [](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "Connect error: " << errorString << " : " << errorNumber;
            }));

        commandStartEvent.publish();
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

    void MariaDBConnection::execute(MariaDBCommand* mariaDBCommand) {
        commandQueue.push_back(mariaDBCommand);

        if (connected && currentCommand == nullptr && commandQueue.size() == 1) {
            commandStartEvent.publish();
        }
    }

    void MariaDBConnection::executeAsNext(MariaDBCommand* mariaDBCommand) {
        commandCompleted();

        commandQueue.push_front(mariaDBCommand);
    }

    void MariaDBConnection::commandStart(const utils::Timeval& currentTime) {
        if (!commandQueue.empty()) {
            currentCommand = commandQueue.front();

            int status = currentCommand->start(mysql, currentTime);

            checkStatus(status);
        } else {
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
        } else if ((status & MYSQL_WAIT_READ) != 0 && commandQueue.empty()) {
            VLOG(0) << "Read-Event but no command in queue: Disabling EventReceivers";

            ReadEventReceiver::disable();
            WriteEventReceiver::disable();
            ExceptionalConditionEventReceiver::disable();
        }
    }

    void MariaDBConnection::commandCompleted() {
        commandQueue.pop_front();

        if (currentCommand != nullptr) {
            delete currentCommand;
            currentCommand = nullptr;
        }
    }

    void MariaDBConnection::unmanaged() {
        mariaDBClient = nullptr;
    }

    void MariaDBConnection::setFd(int status) {
        if ((status & MYSQL_WAIT_READ) != 0 || (status & MYSQL_WAIT_WRITE) != 0 || (status & MYSQL_WAIT_EXCEPT) != 0 ||
            (status == 0 && !currentCommand->error())) {
            fd = mysql_get_socket(mysql);

            ReadEventReceiver::enable(fd);
            WriteEventReceiver::enable(fd);
            ExceptionalConditionEventReceiver::enable(fd);

            WriteEventReceiver::suspend();
            ExceptionalConditionEventReceiver::suspend();

            connected = true;
        } else {
            VLOG(0) << "Mysql setFd-Error: " << mysql_error(mysql) << ", " << mysql_errno(mysql);
            error = true;
        }
    }

    void MariaDBConnection::checkStatus(int status) {
        if (status == 0) {
            if (connected) {
                if (!currentCommand->error()) {
                    currentCommand->commandCompleted();
                } else {
                    currentCommand->commandError(mysql_error(mysql), mysql_errno(mysql));
                    commandCompleted();
                }
                commandStartEvent.publish();
            } else if (error) {
                currentCommand->commandError(mysql_error(mysql), mysql_errno(mysql));
                commandCompleted();
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

    MariaDBCommandStartEvent::MariaDBCommandStartEvent(const std::string& name, MariaDBConnection* mariaDBConnection)
        : core::EventReceiver(name)
        , mariaDBConnection(mariaDBConnection) {
    }

    MariaDBCommandStartEvent::~MariaDBCommandStartEvent() {
        published = false;
        core::EventReceiver::unPublish();
    }

    void MariaDBCommandStartEvent::publish() {
        if (!published) {
            published = true;
            core::EventReceiver::publish();
        }
    }

    void MariaDBCommandStartEvent::dispatch([[maybe_unused]] const utils::Timeval& currentTime) {
        published = false;
        mariaDBConnection->commandStart(currentTime);
    }

} // namespace database::mariadb
