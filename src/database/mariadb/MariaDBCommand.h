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

#include "utils/Timeval.h"

#include <functional>
#include <string> // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    class MariaDBCommand {
    public:
        MariaDBCommand(MariaDBConnection* mariaDBConnection,
                       const std::string& name,
                       const std::function<void(const std::string&, unsigned int)>& onError);
        virtual ~MariaDBCommand() = default;

        const std::string& commandName();

        int commandStart(MYSQL* mysql, const utils::Timeval& currentTime);

    private:
        virtual int commandStart() = 0;

    public:
        virtual int commandContinue(int status) = 0;
        virtual void commandCompleted() = 0;
        virtual void commandError(const std::string& errorString, unsigned int errorNumber) = 0;
        virtual std::string commandInfo() = 0;

    protected:
        std::string name;
        MariaDBConnection* mariaDBConnection;

        MYSQL* mysql = nullptr;

        utils::Timeval startTime = 0;

        const std::function<void(const std::string&, unsigned int)> onError;
    };

} // namespace database::mariadb

#endif // DATABASE_MARIADB_MARIADBCOMMAND
