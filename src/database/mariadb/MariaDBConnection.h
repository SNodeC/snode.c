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

#ifndef MARIADBCONNECTION_H
#define MARIADBCONNECTION_H

#include "core/eventreceiver/ExceptionalConditionEventReceiver.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/eventreceiver/WriteEventReceiver.h"
#include "database/mariadb/MariaDBCommand.h"
#include "database/mariadb/MariaDBConnectionDetails.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {
    class MariaDBClient;
}

namespace database::mariadb {

    class MariaDBConnection
        : core::eventreceiver::ReadEventReceiver
        , core::eventreceiver::WriteEventReceiver
        , core::eventreceiver::ExceptionalConditionEventReceiver {
    public:
        explicit MariaDBConnection(MariaDBClient* mariaDBClient, const MariaDBConnectionDetails& connectionDetails);
        MariaDBConnection(const MariaDBConnection&) = delete;

        ~MariaDBConnection();

        MariaDBConnection& operator=(const MariaDBConnection&) = delete;

        void executeCommand(MariaDBCommand* mariaDBCommand);
        void continueCommand(int status);

        void unmanaged();

    protected:
        void checkStatus(int status);

        void readEvent() override;
        void writeEvent() override;
        void outOfBandEvent() override;

        void readTimeout() override;
        void writeTimeout() override;
        void outOfBandTimeout() override;

        void unobservedEvent() override;

    private:
        MariaDBClient* mariaDBClient;
        MYSQL* mysql;
        bool managed;
        MariaDBConnectionDetails connectionDetails;

        std::list<MariaDBCommand*> commandQueue;

        MariaDBCommand* currentCommand = nullptr;
        bool connected = false;
    };

} // namespace database::mariadb

#endif // MARIADBCONNECTION_H
