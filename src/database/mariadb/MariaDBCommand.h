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

#ifndef DATABASE_MARIADB_MARIADBCOMMAND
#define DATABASE_MARIADB_MARIADBCOMMAND

namespace database::mariadb {
    class MariaDBConnection;
}

typedef struct st_mysql MYSQL;

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string> // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    class MariaDBCommand {
    public:
        MariaDBCommand(MariaDBConnection* mariaDBConnection);
        virtual ~MariaDBCommand() = default;

        virtual int start(MYSQL* mysql) = 0;
        virtual int cont(MYSQL* mysql, int status) = 0;

        virtual bool error() = 0;

        virtual void commandCompleted() = 0;
        virtual void commandError(const std::string& errorString, unsigned int errorNumber) = 0;

    protected:
        MariaDBConnection* mariaDBConnection;
    };

} // namespace database::mariadb

#endif // DATABASE_MARIADB_MARIADBCOMMAND
