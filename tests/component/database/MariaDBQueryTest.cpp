/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "core/SNodeC.h"
#include "database/mariadb/MariaDBClient.h"
#include "database/mariadb/commands/async/MariaDBQueryCommand.h"
#include "support/TestResult.h"

#include <fstream>
#include <memory>
#include <string>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<database::mariadb::commands::async::MariaDBQueryCommand>);

int main(int argc, char* argv[]) {
    tests::support::TestResult checks;
    int result = EXIT_FAILURE;
    if (argc == 4) {
        const std::string mode = argv[1];
        const bool transaction = mode == "rollback";
        const bool invalid = mode == "following" || transaction;
        std::ofstream("empty.conf").close();
        char configOption[] = "--config-file";
        char configPath[] = "empty.conf";
        char* options[] = {argv[0], configOption, configPath};
        core::SNodeC::init(3, options);
        MYSQL* reader = mysql_init(nullptr);
        const bool connected = mysql_real_connect(reader, "localhost", argv[3], "", nullptr, 0, argv[2], 0) != nullptr;
        checks.expectTrue(connected, "independent fixture connection");
        if (connected) {
            checks.expectEqual(0, mysql_query(reader, "CREATE DATABASE probe"), "create disposable database");
            checks.expectEqual(
                0, mysql_query(reader, "CREATE TABLE probe.items(id INT PRIMARY KEY) ENGINE=InnoDB"), "create fixture table");
            std::unique_ptr<database::mariadb::MariaDBClient> db;
            database::mariadb::MariaDBCommandSequence* sequence = nullptr;
            int errors = 0, rows = 0, rollbacks = 0, firstRows = 0, firstEnds = 0, metadata = 0;
            bool started = false, finished = false;
            const auto unexpected = [&](const std::string&, unsigned int) {
                checks.expectTrue(false, "unexpected native error");
                db.reset();
            };
            const auto row = [&](MYSQL_ROW value) {
                if (value != nullptr) {
                    ++rows;
                    checks.expectTrue(std::string(value[0]) == "42", "following user query is preserved");
                } else {
                    finished = true;
                    db.reset();
                }
            };
            const auto failure = [&](const std::string& message, unsigned int code) {
                ++errors;
                checks.expectEqual(1054, static_cast<int>(code), "original query error code survives cleanup");
                checks.expectTrue(message.find("missing") != std::string::npos, "original query error text survives cleanup");
                if (transaction) {
                    sequence->query(
                        "SELECT COUNT(*) FROM items",
                        [&](MYSQL_ROW value) {
                            if (value != nullptr) {
                                checks.expectTrue(std::string(value[0]) == "1", "driver leaves rollback to the caller");
                            } else {
                                sequence->rollback(
                                    [&]() {
                                        ++rollbacks;
                                        sequence->endTransactions(
                                            [&]() {
                                                sequence->query("SELECT 42", row, unexpected);
                                            },
                                            unexpected);
                                    },
                                    unexpected);
                            }
                        },
                        unexpected);
                }
            };
            const database::mariadb::MariaDBConnectionDetails details{.connectionName = "query-error-test",
                                                                      .hostname = "localhost",
                                                                      .username = argv[3],
                                                                      .password = "",
                                                                      .database = "probe",
                                                                      .port = 0,
                                                                      .socket = argv[2],
                                                                      .flags = 0};
            db = std::make_unique<database::mariadb::MariaDBClient>(
                details,
                [&](const database::mariadb::MariaDBState& state) {
                    if (state.connected && !started) {
                        started = true;
                        if (transaction) {
                            sequence = &db->startTransactions(
                                [&]() {
                                    sequence->exec(
                                        "INSERT INTO items VALUES (1)",
                                        [&]() {
                                            sequence->query(
                                                "SELECT missing FROM items",
                                                [&](MYSQL_ROW) {
                                                    checks.expectTrue(false, "failed query must not deliver rows or EOF");
                                                },
                                                failure);
                                        },
                                        unexpected);
                                },
                                unexpected);
                        } else if (!invalid) {
                            const std::string sql = mode == "rows"    ? "SELECT 1 UNION ALL SELECT 2"
                                                    : mode == "empty" ? "SELECT id FROM items"
                                                                      : "SET @query_test = 1";
                            db->query(
                                  sql,
                                  [&](MYSQL_ROW value) {
                                      if (value != nullptr) {
                                          ++firstRows;
                                          checks.expectTrue(std::string(value[0]) == std::to_string(firstRows), "rows remain ordered");
                                          db->fieldCount(
                                              [&](unsigned int count) {
                                                  checks.expectEqual(1, static_cast<int>(count), "row callback metadata remains available");
                                                  ++metadata;
                                              },
                                              unexpected);
                                      } else {
                                          ++firstEnds;
                                      }
                                  },
                                  unexpected)
                                .query("SELECT 42", row, unexpected);
                        } else {
                            db->query(
                                  "SELECT missing FROM items",
                                  [&](MYSQL_ROW) {
                                      checks.expectTrue(false, "failed query must not deliver rows or EOF");
                                  },
                                  failure)
                                .query("SELECT 42", row, unexpected);
                        }
                    } else if (state.error != 0) {
                        unexpected(state.errorMessage, state.error);
                    }
                },
                "database");
            db->connect();
            core::SNodeC::start();
            checks.expectTrue(finished, "following query completes");
            checks.expectEqual(invalid ? 1 : 0, errors, "one error per failed public query; no caller-side duplicate guard");
            if (!invalid) {
                checks.expectEqual(mode == "rows" ? 2 : 0, firstRows, "first query returns its exact rows");
                checks.expectEqual(mode == "rows" ? 2 : 0, metadata, "metadata callbacks complete");
                checks.expectEqual(1, firstEnds, "first query has one end-of-results callback");
            }
            checks.expectEqual(1, rows, "healthy following query delivers one row");
            checks.expectEqual(transaction ? 1 : 0, rollbacks, "only caller requests rollback, exactly once");
            checks.expectEqual(0, mysql_query(reader, "SELECT COUNT(*) FROM probe.items"), "independent final read");
            MYSQL_RES* records = mysql_store_result(reader);
            MYSQL_ROW value = records != nullptr ? mysql_fetch_row(records) : nullptr;
            checks.expectTrue(value != nullptr && std::string(value[0]) == "0", "no unintended durable write");
            mysql_free_result(records);
            db.reset();
        }
        mysql_close(reader);
        result = checks.processResult();
    }
    return result;
}
