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

#ifndef MARIA_DB_CLIENT
#define MARIA_DB_CLIENT

#include "database/mariadb/MariaDBConnectionDetails.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {
    class MariaDBConnection;
    class MariaDBCommand;
} // namespace database::mariadb

namespace database::mariadb {

    class MariaDBClient {
    public:
        explicit MariaDBClient(const MariaDBConnectionDetails& details);
        ~MariaDBClient();

        void query(const std::string& sql, const std::function<void()>& onQuery, const std::function<void(const std::string&)> onError);

        void disconnect(const std::function<void()>& onDisconnect);

    private:
        void execute(MariaDBCommand* mariaDBCommand);

        void connectionVanished();
        MariaDBConnection* mariaDBConnection;

        friend class MariaDBConnection;
    };

} // namespace database::mariadb

#endif // MARIA_DB_CLIENT
