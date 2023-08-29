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

#include "database/mariadb/MariaDBConnection.h"

#include "database/mariadb/MariaDBClient.h"
#include "database/mariadb/commands/async/MariaDBConnectCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/SNodeC.h"
#include "log/Logger.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    MariaDBConnection::MariaDBConnection(MariaDBClient* mariaDBClient, const MariaDBConnectionDetails& connectionDetails)
        : ReadEventReceiver("MariaDBConnectionRead", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , WriteEventReceiver("MariaDBConnectionWrite", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , ExceptionalConditionEventReceiver("MariaDBConnectionExceptional", core::DescriptorEventReceiver::TIMEOUT::DISABLE)
        , mariaDBClient(mariaDBClient)
        , mysql(mysql_init(nullptr))
        , commandStartEvent("MariaDBCommandStartEvent", this) {
        mysql_options(mysql, MYSQL_OPT_NONBLOCK, nullptr);

        execute_async(std::move(MariaDBCommandSequence().execute_async(new database::mariadb::commands::async::MariaDBConnectCommand(
            connectionDetails,
            [this]() -> void {
                if (mysql_errno(mysql) == 0) {
                    int fd = mysql_get_socket(mysql);

                    VLOG(0) << "Got valid descriptor: " << fd;

                    ReadEventReceiver::enable(fd);
                    WriteEventReceiver::enable(fd);
                    ExceptionalConditionEventReceiver::enable(fd);

                    ReadEventReceiver::suspend();
                    WriteEventReceiver::suspend();
                    ExceptionalConditionEventReceiver::suspend();

                    connected = true;
                } else {
                    VLOG(0) << "Got no valid descriptor: " << mysql_error(mysql) << ", " << mysql_errno(mysql);
                }
            },
            []() -> void {
                VLOG(0) << "Connect success";
            },
            [](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "Connect error: " << errorString << " : " << errorNumber;
            }))));
    }

    MariaDBConnection::~MariaDBConnection() {
        for (MariaDBCommandSequence& mariaDBCommandSequence : commandSequenceQueue) {
            for (MariaDBCommand* mariaDBCommand : mariaDBCommandSequence.sequence()) {
                if (core::SNodeC::state() == core::State::RUNNING && connected) {
                    mariaDBCommand->commandError(mysql_error(mysql), mysql_errno(mysql));
                }

                delete mariaDBCommand;
            }
        }

        if (mariaDBClient != nullptr) {
            mariaDBClient->connectionVanished();
        }

        mysql_close(mysql);
        mysql_library_end();
    }

    MariaDBCommandSequence& MariaDBConnection::execute_async(MariaDBCommandSequence&& commandSequence) {
        if (currentCommand == nullptr && commandSequenceQueue.empty()) {
            commandStartEvent.span();
        }

        commandSequenceQueue.emplace_back(std::move(commandSequence));

        return commandSequenceQueue.back();
    }

    void MariaDBConnection::execute_sync(MariaDBCommand* mariaDBCommand) {
        mariaDBCommand->commandStart(mysql, utils::Timeval::currentTime());

        if (mysql_errno(mysql) == 0) {
            if (mariaDBCommand->commandCompleted()) {
            }
        } else {
            mariaDBCommand->commandError(mysql_error(mysql), mysql_errno(mysql));
        }

        delete mariaDBCommand;
    }

    void MariaDBConnection::commandStart(const utils::Timeval& currentTime) {
        if (!commandSequenceQueue.empty()) {
            currentCommand = commandSequenceQueue.front().nextCommand();

            currentCommand->setMariaDBConnection(this);
            checkStatus(currentCommand->commandStart(mysql, currentTime));
        } else if (mariaDBClient != nullptr) {
            if (ReadEventReceiver::isSuspended()) {
                ReadEventReceiver::resume();
            }
        } else {
            ReadEventReceiver::disable();
            WriteEventReceiver::disable();
            ExceptionalConditionEventReceiver::disable();
        }
    }

    void MariaDBConnection::commandContinue(int status) {
        if (currentCommand != nullptr) {
            checkStatus(currentCommand->commandContinue(status));
        } else if ((status & MYSQL_WAIT_READ) != 0 && commandSequenceQueue.empty()) {
            ReadEventReceiver::disable();
            WriteEventReceiver::disable();
            ExceptionalConditionEventReceiver::disable();
        }
    }

    void MariaDBConnection::commandCompleted() {
        VLOG(0) << "Completed: " << currentCommand->commandInfo();
        commandSequenceQueue.front().commandCompleted();

        if (commandSequenceQueue.front().empty()) {
            commandSequenceQueue.pop_front();
        }

        delete currentCommand;
        currentCommand = nullptr;
    }

    void MariaDBConnection::unmanaged() {
        mariaDBClient = nullptr;
    }

    void MariaDBConnection::checkStatus(int status) {
        if (connected) {
            if ((status & MYSQL_WAIT_READ) != 0) {
                if (ReadEventReceiver::isSuspended()) {
                    ReadEventReceiver::resume();
                }
            } else if (!ReadEventReceiver::isSuspended()) {
                ReadEventReceiver::suspend();
            }

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
                //                WriteEventReceiver::setTimeout(mysql_get_timeout_value(mysql));
                //                ExceptionalConditionEventReceiver::setTimeout(mysql_get_timeout_value(mysql));
            } else {
                ReadEventReceiver::setTimeout(core::DescriptorEventReceiver::TIMEOUT::DEFAULT);
                //                WriteEventReceiver::setTimeout(core::DescriptorEventReceiver::TIMEOUT::DEFAULT);
                //                ExceptionalConditionEventReceiver::setTimeout(core::DescriptorEventReceiver::TIMEOUT::DEFAULT);
            }

            if (status == 0) {
                if (mysql_errno(mysql) == 0) {
                    if (currentCommand->commandCompleted()) {
                        commandCompleted();
                    }
                } else {
                    currentCommand->commandError(mysql_error(mysql), mysql_errno(mysql));
                    commandCompleted();
                }
                commandStartEvent.span();
            }
        } else {
            currentCommand->commandError(mysql_error(mysql), mysql_errno(mysql));
            commandCompleted();
            delete this;
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

    void MariaDBCommandStartEvent::onEvent(const utils::Timeval& currentTime) {
        mariaDBConnection->commandStart(currentTime);
    }

    void MariaDBCommandStartEvent::destruct() {
        delete mariaDBConnection;
    }

} // namespace database::mariadb
