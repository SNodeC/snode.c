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

#include "database/mariadb/MariaDBCommandSequence.h"

#include "database/mariadb/MariaDBCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    database::mariadb::MariaDBCommandSequence::MariaDBCommandSequence(database::mariadb::MariaDBConnection* mariaDBConnection,
                                                                      MariaDBCommand* mariaDBCommand)
        : mariaDBConnection(mariaDBConnection) {
        mariaDBCommand->setMariaDBConnection(mariaDBConnection);
        commandSequence.push_back(mariaDBCommand);
    }

    std::deque<MariaDBCommand*>& database::mariadb::MariaDBCommandSequence::sequence() {
        return commandSequence;
    }

    MariaDBCommand* MariaDBCommandSequence::nextCommand() {
        return commandSequence.front();
    }

    void MariaDBCommandSequence::commandCompleted() {
        commandSequence.pop_front();
    }

    bool MariaDBCommandSequence::empty() {
        return commandSequence.empty();
    }

    MariaDBCommandSequence& database::mariadb::MariaDBCommandSequence::execute(MariaDBCommand* mariaDBCommand) {
        mariaDBCommand->setMariaDBConnection(mariaDBConnection);
        commandSequence.push_back(mariaDBCommand);

        return *this;
    }

} // namespace database::mariadb
