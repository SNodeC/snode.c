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

#ifndef DATABASE_MARIADB_MARIADBCOMMANDSEQUENCE
#define DATABASE_MARIADB_MARIADBCOMMANDSEQUENCE

#include "database/mariadb/MariaDBClientASyncAPI.h"
#include "database/mariadb/MariaDBClientSyncAPI.h"

namespace database::mariadb {
    class MariaDBCommand;
    class MariaDBCommandSync;
} // namespace database::mariadb

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <deque>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    class MariaDBCommandSequence
        : public MariaDBClientASyncAPI
        , public MariaDBClientSyncAPI {
    public:
        MariaDBCommandSequence() = default;
        MariaDBCommandSequence(MariaDBCommandSequence&& mariaDBCommandSequence) = default;
        ~MariaDBCommandSequence() override = default;

        MariaDBCommandSequence& operator=(MariaDBCommandSequence&& mariaDBCommandSequence) = default;

    private:
        MariaDBCommandSequence& execute_async(MariaDBCommand* mariaDBCommand) final;
        void execute_sync(MariaDBCommandSync* mariaDBCommand) final;

        std::deque<MariaDBCommand*>& sequence();

        MariaDBCommand* nextCommand();
        void commandCompleted();
        bool empty();

        std::deque<MariaDBCommand*> commandSequence;

        friend class MariaDBConnection;
        friend class MariaDBClient;
        friend class MariaDBClientASyncAPI;
    };

} // namespace database::mariadb

#endif // DATABASE_MARIADB_MARIADBCOMMANDSEQUENCE
