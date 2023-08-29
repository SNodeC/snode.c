/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "database/mariadb/MariaDBCommandSync.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

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

    MariaDBCommandSequence& MariaDBCommandSequence::execute_async(MariaDBCommand* mariaDBCommand) {
        commandSequence.push_back(mariaDBCommand);

        return *this;
    }

    void MariaDBCommandSequence::execute_sync(MariaDBCommandSync* mariaDBCommand) {
        commandSequence.push_back(mariaDBCommand);
    }

} // namespace database::mariadb
