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

#include "database/mariadb/MariaDBClientAPI.h"

#include "database/mariadb/commands/MariaDBAutoCommitCommand.h"
#include "database/mariadb/commands/MariaDBCommitCommand.h"
#include "database/mariadb/commands/MariaDBInsertCommand.h"
#include "database/mariadb/commands/MariaDBQueryCommand.h"
#include "database/mariadb/commands/MariaDBRollbackCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    MariaDBCommandSequence& MariaDBClientAPI::query(const std::string& sql,
                                                    const std::function<void(const MYSQL_ROW)>& onQuery,
                                                    const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute<database::mariadb::commands::MariaDBQueryCommand>(sql, onQuery, onError);
    }

    MariaDBCommandSequence& MariaDBClientAPI::exec(const std::string& sql,
                                                   const std::function<void(int)>& onQuery,
                                                   const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute<database::mariadb::commands::MariaDBInsertCommand>(sql, onQuery, onError);
    }

    MariaDBCommandSequence& MariaDBClientAPI::startTransactions(const std::function<void()>& onSet,
                                                                const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute<database::mariadb::commands::MariaDBAutoCommitCommand>(false, onSet, onError);
    }

    MariaDBCommandSequence& MariaDBClientAPI::endTransactions(const std::function<void()>& onUnset,
                                                              const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute<database::mariadb::commands::MariaDBAutoCommitCommand>(true, onUnset, onError);
    }

    MariaDBCommandSequence& MariaDBClientAPI::commit(const std::function<void()>& onCommit,
                                                     const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute<database::mariadb::commands::MariaDBCommitCommand>(onCommit, onError);
    }

    MariaDBCommandSequence& MariaDBClientAPI::rollback(const std::function<void()>& onRollback,
                                                       const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute<database::mariadb::commands::MariaDBRollbackCommand>(onRollback, onError);
    }

    template <typename Command>
    MariaDBCommandSequence& MariaDBClientAPI::execute(const auto& arg,
                                                      const auto& onSuccess,
                                                      const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute(new Command(arg, onSuccess, onError));
    }

    template <typename Command>
    MariaDBCommandSequence& MariaDBClientAPI::execute(const auto& onSuccess,
                                                      const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute(new Command(onSuccess, onError));
    }

} // namespace database::mariadb
