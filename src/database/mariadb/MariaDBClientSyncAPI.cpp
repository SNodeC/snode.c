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

#include "database/mariadb/MariaDBClientSyncAPI.h"

#include "database/mariadb/MariaDBCommandSequence.h"
#include "database/mariadb/commands/async/MariaDBAutoCommitCommand.h"
#include "database/mariadb/commands/async/MariaDBCommitCommand.h"
#include "database/mariadb/commands/async/MariaDBExecCommand.h"
#include "database/mariadb/commands/async/MariaDBFetchRowCommand.h"
#include "database/mariadb/commands/async/MariaDBFreeResultCommand.h"
#include "database/mariadb/commands/async/MariaDBQueryCommand.h"
#include "database/mariadb/commands/async/MariaDBRollbackCommand.h"
#include "database/mariadb/commands/sync/MariaDBAffectedRowsCommand.h"
#include "database/mariadb/commands/sync/MariaDBFieldCountCommand.h"
#include "database/mariadb/commands/sync/MariaDBUseResultCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    void MariaDBClientSyncAPI::affectedRows(const std::function<void(int)>& onAffectedRows,
                                            const std::function<void(const std::string&, unsigned int)>& onError) {
        execute_sync(new database::mariadb::commands::sync::MariaDBAffectedRowsCommand(onAffectedRows, onError));
    }

    void MariaDBClientSyncAPI::fieldCount(const std::function<void(unsigned int)>& onFieldCount,
                                          const std::function<void(const std::string&, unsigned int)>& onError) {
        execute_sync(new database::mariadb::commands::sync::MariaDBFieldCountCommand(onFieldCount, onError));
    }

} // namespace database::mariadb
