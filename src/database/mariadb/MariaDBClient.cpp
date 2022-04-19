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

#include "database/mariadb/MariaDBClient.h"

#include "database/mariadb/MariaDBConnection.h"
#include "database/mariadb/commands/MariaDBAutoCommitCommand.h"
#include "database/mariadb/commands/MariaDBCommitCommand.h"
#include "database/mariadb/commands/MariaDBInsertCommand.h"
#include "database/mariadb/commands/MariaDBQueryCommand.h"
#include "database/mariadb/commands/MariaDBRollbackCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    MariaDBClient::MariaDBClient(const MariaDBConnectionDetails& details)
        : details(details) {
    }

    MariaDBClient::~MariaDBClient() {
        if (mariaDBConnection != nullptr) {
            mariaDBConnection->unmanaged();
        }
    }

    void MariaDBClient::query(const std::string& sql,
                              const std::function<void(const MYSQL_ROW)>& onQuery,
                              const std::function<void(const std::string&, unsigned int)>& onError) {
        execute<database::mariadb::commands::MariaDBQueryCommand>(sql, onQuery, onError);
    }

    void MariaDBClient::insert(const std::string& sql,
                               const std::function<void()>& onQuery,
                               const std::function<void(const std::string&, unsigned int)>& onError) {
        execute<database::mariadb::commands::MariaDBInsertCommand>(sql, onQuery, onError);
    }

    void MariaDBClient::startTransactions(const std::function<void()>& onSet,
                                          const std::function<void(const std::string&, unsigned int)>& onError) {
        execute<database::mariadb::commands::MariaDBAutoCommitCommand>(false, onSet, onError);
    }

    void MariaDBClient::endTransactions(const std::function<void()>& onUnset,
                                        const std::function<void(const std::string&, unsigned int)>& onError) {
        execute<database::mariadb::commands::MariaDBAutoCommitCommand>(true, onUnset, onError);
    }

    void MariaDBClient::commit(const std::function<void()>& onCommit,
                               const std::function<void(const std::string&, unsigned int)>& onError) {
        execute<database::mariadb::commands::MariaDBCommitCommand>(onCommit, onError);
    }

    void MariaDBClient::rollback(const std::function<void()>& onRollback,
                                 const std::function<void(const std::string&, unsigned int)>& onError) {
        execute<database::mariadb::commands::MariaDBRollbackCommand>(onRollback, onError);
    }

    template <typename Command>
    void
    MariaDBClient::execute(const auto& arg, const auto& onSuccess, const std::function<void(const std::string&, unsigned int)>& onError) {
        if (mariaDBConnection == nullptr) {
            mariaDBConnection = new MariaDBConnection(this, details);
        }
        if (mariaDBConnection != nullptr) {
            mariaDBConnection->execute(new Command(mariaDBConnection, arg, onSuccess, onError));
        }
    }

    template <typename Command>
    void MariaDBClient::execute(const auto& onSuccess, const std::function<void(const std::string&, unsigned int)>& onError) {
        if (mariaDBConnection == nullptr) {
            mariaDBConnection = new MariaDBConnection(this, details);
        }
        if (mariaDBConnection != nullptr) {
            mariaDBConnection->execute(new Command(mariaDBConnection, onSuccess, onError));
        }
    }

    void MariaDBClient::disconnect([[maybe_unused]] const std::function<void()>& onDisconnect) {
        VLOG(0) << "Disconnect not implemented yet.";
    }

    void MariaDBClient::connectionVanished() {
        mariaDBConnection = nullptr;
    }

} // namespace database::mariadb
