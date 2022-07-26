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

#ifndef DATABASE_MARIADB_COMMANDS_SYNC_MARIADBFIELDCOUNTCOMMAND
#define DATABASE_MARIADB_COMMANDS_SYNC_MARIADBFIELDCOUNTCOMMAND

#include "database/mariadb/MariaDBCommandSync.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands::sync {

    class MariaDBFieldCountCommand : public MariaDBCommandSync {
    public:
        MariaDBFieldCountCommand(const std::function<void(unsigned int)>& onFieldCount,
                                 const std::function<void(const std::string&, unsigned int)>& onError);

        int commandStart() override;
        bool commandCompleted() override;
        void commandError(const std::string& errorString, unsigned int errorNumber) override;

    protected:
        unsigned int fieldCount = 0;

        const std::function<void(unsigned int)> onFieldCount;
    };

} // namespace database::mariadb::commands::sync

#endif // DATABASE_MARIADB_COMMANDS_SYNC_MARIADBFIELDCOUNTCOMMAND
