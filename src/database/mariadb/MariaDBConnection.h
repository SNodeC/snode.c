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

#ifndef DATABASE_MARIADB_MARIADBCONNECTION
#define DATABASE_MARIADB_MARIADBCONNECTION

#include "core/eventreceiver/ExceptionalConditionEventReceiver.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/eventreceiver/WriteEventReceiver.h"
#include "database/mariadb/MariaDBCommandSequence.h" // IWYU pragma: export

namespace database::mariadb {
    class MariaDBCommand;
    class MariaDBClient;
    class MariaDBConnection;
    struct MariaDBConnectionDetails;
} // namespace database::mariadb

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <deque>
#include <mysql.h>
#include <string> // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    class MariaDBCommandStartEvent : private core::EventReceiver {
    public:
        MariaDBCommandStartEvent(const std::string& name, MariaDBConnection* mariaDBConnection);
        ~MariaDBCommandStartEvent();

        using core::EventReceiver::publish;

        void publish();

    private:
        void event(const utils::Timeval& currentTime) override;

        MariaDBConnection* mariaDBConnection = nullptr;

        //        bool published = false;
    };

    class MariaDBConnection
        : private core::eventreceiver::ReadEventReceiver
        , private core::eventreceiver::WriteEventReceiver
        , private core::eventreceiver::ExceptionalConditionEventReceiver {
    public:
        explicit MariaDBConnection(MariaDBClient* mariaDBClient, const MariaDBConnectionDetails& connectionDetails);
        MariaDBConnection(const MariaDBConnection&) = delete;

        ~MariaDBConnection();

        MariaDBConnection& operator=(const MariaDBConnection&) = delete;

        MariaDBCommandSequence& execute(MariaDBCommandSequence&& commandSequence);
        void executeAsNext(MariaDBCommand* mariaDBCommand);

        void commandStart(const utils::Timeval& currentTime);
        void commandContinue(int status);
        void commandCompleted();

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
        MariaDBCommandStartEvent commandStartEvent;

        MYSQL* mysql;

        std::deque<MariaDBCommandSequence> commandSequenceQueue;

        MariaDBCommand* currentCommand = nullptr;

        bool connected = false;
    };

} // namespace database::mariadb

#endif // DATABASE_MARIADB_MARIADBCONNECTION
