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

namespace utils {
    class Timeval;
}

#include <deque>
#include <mysql.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    class MariaDBCommandStartEvent : private core::EventReceiver {
    public:
        MariaDBCommandStartEvent(const std::string& name, MariaDBConnection* mariaDBConnection) noexcept;

        using core::EventReceiver::span;

    private:
        void onEvent(const utils::Timeval& currentTime) noexcept override;

        void destruct() noexcept override;

        MariaDBConnection* mariaDBConnection = nullptr;
    };

    class MariaDBConnection
        : private core::eventreceiver::ReadEventReceiver
        , private core::eventreceiver::WriteEventReceiver
        , private core::eventreceiver::ExceptionalConditionEventReceiver {
    public:
        MariaDBConnection(MariaDBClient* mariaDBClient, const MariaDBConnectionDetails& connectionDetails) noexcept;
        MariaDBConnection(const MariaDBConnection&) = delete;

        ~MariaDBConnection() noexcept override;

        MariaDBConnection& operator=(const MariaDBConnection&) = delete;

        MariaDBCommandSequence& execute_async(MariaDBCommandSequence&& commandSequence) noexcept;
        void execute_sync(MariaDBCommand* mariaDBCommand) noexcept;

        void commandStart(const utils::Timeval& currentTime) noexcept;
        void commandContinue(int status) noexcept;
        void commandCompleted() noexcept;

        void unmanaged() noexcept;

    protected:
        void checkStatus(int status);

        void readEvent() noexcept override;
        void writeEvent() noexcept override;
        void outOfBandEvent() noexcept override;

        void readTimeout() noexcept override;
        void writeTimeout() noexcept override;
        void outOfBandTimeout() noexcept override;

        void unobservedEvent() noexcept override;

    private:
        MariaDBClient* mariaDBClient;
        MYSQL* mysql;

        std::deque<MariaDBCommandSequence> commandSequenceQueue;

        MariaDBCommand* currentCommand = nullptr;
        bool connected = false;

        MariaDBCommandStartEvent commandStartEvent;
    };

} // namespace database::mariadb

#endif // DATABASE_MARIADB_MARIADBCONNECTION
