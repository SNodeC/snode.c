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

#ifndef DATABASE_MARIADB_COMMANDS_ASYNC_MARIADBROLLBACKCOMMAND
#define DATABASE_MARIADB_COMMANDS_ASYNC_MARIADBROLLBACKCOMMAND

#include "database/mariadb/MariaDBCommandASync.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb::commands::async {

    class MariaDBRollbackCommand : public MariaDBCommandASync {
    public:
        MariaDBRollbackCommand(const std::function<void(void)>& onRollback,
                               const std::function<void(const std::string&, unsigned int)>& onError);

    private:
        int commandStart() override;
        int commandContinue(int status) override;
        bool commandCompleted() override;
        void commandError(const std::string& errorString, unsigned int errorNumber) override;

        my_bool ret = false;

        const std::function<void(void)> onRollback;
    };

} // namespace database::mariadb::commands::async

#endif // DATABASE_MARIADB_COMMANDS_ASYNC_MARIADBAUTOCOMMITCOMMAND
