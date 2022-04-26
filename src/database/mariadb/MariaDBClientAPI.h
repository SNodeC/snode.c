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

#ifndef DATABASE_MARIADB_MARIADBCLIENTAPI
#define DATABASE_MARIADB_MARIADBCLIENTAPI

namespace database::mariadb {
    class MariaDBCommand;
    class MariaDBCommandSequence;
} // namespace database::mariadb

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <mysql.h> // IWYU pragma: export
#include <string>

// IWYU pragma: no_include "mysql.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    class MariaDBClientAPI {
    public:
        virtual ~MariaDBClientAPI() = default;

        MariaDBCommandSequence& query(const std::string& sql,
                                      const std::function<void(const MYSQL_ROW)>& onQuery,
                                      const std::function<void(const std::string&, unsigned int)>& onError);
        MariaDBCommandSequence& exec(const std::string& sql,
                                     const std::function<void(void)>& onQuery,
                                     const std::function<void(const std::string&, unsigned int)>& onError);

        MariaDBCommandSequence& startTransactions(const std::function<void(void)>&,
                                                  const std::function<void(const std::string&, unsigned int)>& onError);
        MariaDBCommandSequence& endTransactions(const std::function<void(void)>&,
                                                const std::function<void(const std::string&, unsigned int)>& onError);

        MariaDBCommandSequence& commit(const std::function<void(void)>&,
                                       const std::function<void(const std::string&, unsigned int)>& onError);
        MariaDBCommandSequence& rollback(const std::function<void(void)>&,
                                         const std::function<void(const std::string&, unsigned int)>& onError);

        MariaDBCommandSequence& affectedRows(const std::function<void(int)>&,
                                             const std::function<void(const std::string&, unsigned int)>& onErro);

    protected:
        virtual MariaDBCommandSequence& execute(MariaDBCommand* mariaDBCommand) = 0;

        MYSQL_RES* lastResult = nullptr;
    };

} // namespace database::mariadb

#endif // DATABASE_MARIADB_MARIADBCLIENTAPI
