/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "database/mariadb/MariaDBClientASyncAPI.h"

#include "database/mariadb/MariaDBCommandSequence.h"
#include "database/mariadb/commands/async/MariaDBAutoCommitCommand.h"
#include "database/mariadb/commands/async/MariaDBCommitCommand.h"
#include "database/mariadb/commands/async/MariaDBExecCommand.h"
#include "database/mariadb/commands/async/MariaDBFetchRowCommand.h"
#include "database/mariadb/commands/async/MariaDBFreeResultCommand.h"
#include "database/mariadb/commands/async/MariaDBQueryCommand.h"
#include "database/mariadb/commands/async/MariaDBRollbackCommand.h"
#include "database/mariadb/commands/sync/MariaDBUseResultCommand.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <mysql.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    MariaDBClientASyncAPI::~MariaDBClientASyncAPI() {
    }

    MariaDBCommandSequence& MariaDBClientASyncAPI::query(const std::string& sql,
                                                         const std::function<void(const MYSQL_ROW)>& onQuery,
                                                         const std::function<void(const std::string&, unsigned int)>& onError) {
        MariaDBCommandSequence& sequence = execute_async(new database::mariadb::commands::async::MariaDBQueryCommand(
            sql,
            []() {
            },
            onError));

        return sequence
            .execute_async(new database::mariadb::commands::sync::MariaDBUseResultCommand(
                [&lastResult = sequence.lastResult](MYSQL_RES* result) {
                    lastResult = result;
                },
                onError))
            .execute_async(new database::mariadb::commands::async::MariaDBFetchRowCommand(sequence.lastResult, onQuery, onError))
            .execute_async(new database::mariadb::commands::async::MariaDBFreeResultCommand(
                sequence.lastResult,
                [&lastResult = sequence.lastResult]() {
                    lastResult = nullptr;
                },
                onError));
    }

    MariaDBCommandSequence& MariaDBClientASyncAPI::exec(const std::string& sql,
                                                        const std::function<void(void)>& onExec,
                                                        const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute_async(new database::mariadb::commands::async::MariaDBExecCommand(sql, onExec, onError));
    }

    MariaDBCommandSequence& MariaDBClientASyncAPI::startTransactions(const std::function<void()>& onAutoCommit,
                                                                     const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute_async(new database::mariadb::commands::async::MariaDBAutoCommitCommand(0, onAutoCommit, onError));
    }

    MariaDBCommandSequence& MariaDBClientASyncAPI::endTransactions(const std::function<void()>& onAutoCommit,
                                                                   const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute_async(new database::mariadb::commands::async::MariaDBAutoCommitCommand(1, onAutoCommit, onError));
    }

    MariaDBCommandSequence& MariaDBClientASyncAPI::commit(const std::function<void()>& onCommit,
                                                          const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute_async(new database::mariadb::commands::async::MariaDBCommitCommand(onCommit, onError));
    }

    MariaDBCommandSequence& MariaDBClientASyncAPI::rollback(const std::function<void()>& onRollback,
                                                            const std::function<void(const std::string&, unsigned int)>& onError) {
        return execute_async(new database::mariadb::commands::async::MariaDBRollbackCommand(onRollback, onError));
    }

} // namespace database::mariadb
