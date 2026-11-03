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

#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "database/mariadb/MariaDBClient.h"
#include "support/TestResult.h"

#include <fstream>
#include <functional>
#include <memory>
#include <string>

int main(int argc, char* argv[]) {
    tests::support::TestResult checks;
    int result = EXIT_FAILURE;
    if (argc == 4) {
        const std::string mode = argv[1];
        std::ofstream("empty.conf").close();
        char configOption[] = "--config-file";
        char configPath[] = "empty.conf";
        char* options[] = {argv[0], configOption, configPath};
        core::SNodeC::init(3, options);
        MYSQL* reader = mysql_init(nullptr);
        const bool available = mysql_real_connect(reader, "localhost", argv[3], "", nullptr, 0, argv[2], 0) != nullptr;
        checks.expectTrue(available, "independent fixture reader connects");
        if (available) {
            checks.expectEqual(0, mysql_query(reader, "CREATE DATABASE probe"), "create isolated database");
            checks.expectEqual(
                0, mysql_query(reader, "CREATE TABLE probe.items(id INT PRIMARY KEY) ENGINE=InnoDB"), "create isolated table");
            std::unique_ptr<database::mariadb::MariaDBClient> db;
            database::mariadb::MariaDBCommandSequence* sequence = nullptr;
            int connections = 0, errors = 0, rows = 0, lateCallbacks = 0, writes = 0, ticks = 0;
            bool cancelled = false, started = false, enteredFinished = false;
            const auto abort = [&]() {
                cancelled = true;
                db->abort();
                db->abort();
                if (mode == "reuse") {
                    db->connect();
                } else {
                    db.reset();
                }
            };
            const auto unexpected = [&](const std::string&, unsigned int) {
                if (cancelled) {
                    ++lateCallbacks;
                } else {
                    checks.expectTrue(false, "unexpected command failure");
                    abort();
                }
            };
            const auto forbidden = [&]() {
                ++lateCallbacks;
            };
            const auto queueWrites = [&]() {
                db->startTransactions(forbidden, unexpected)
                    .exec("INSERT INTO items VALUES (999)", forbidden, unexpected)
                    .commit(forbidden, unexpected);
            };
            std::function<void()> later;
            later = [&]() {
                --ticks;
                if (ticks == 0) {
                    abort();
                } else {
                    core::EventReceiver::atNextTick(later);
                }
            };
            const database::mariadb::MariaDBConnectionDetails details{.connectionName = "abort-test",
                                                                      .hostname = "localhost",
                                                                      .username = argv[3],
                                                                      .password = "",
                                                                      .database = "probe",
                                                                      .port = 0,
                                                                      .socket = mode == "connect-error" ? std::string(argv[2]) + ".missing"
                                                                                                        : std::string(argv[2]),
                                                                      .flags = 0};
            db = std::make_unique<database::mariadb::MariaDBClient>(
                details,
                [&](const database::mariadb::MariaDBState& state) {
                    if (cancelled && mode != "reuse") {
                        ++lateCallbacks;
                    } else if (state.error != 0) {
                        ++errors;
                        checks.expectTrue(mode == "connect-error", "only deliberate connect failure is expected");
                        abort();
                    } else if (state.connected) {
                        ++connections;
                        if (mode == "reuse" && cancelled) {
                            db->exec(
                                "INSERT INTO items VALUES (7)",
                                [&]() {
                                    ++writes;
                                    db.reset();
                                },
                                unexpected);
                        } else if (!started) {
                            started = true;
                            if (mode == "connect" || mode == "reuse") {
                                queueWrites();
                                abort();
                                enteredFinished = true;
                            } else if (mode == "drain") {
                                db->startTransactions(
                                      [&]() {
                                          ++writes;
                                      },
                                      unexpected)
                                    .exec(
                                        "INSERT INTO items VALUES (1)",
                                        [&]() {
                                            ++writes;
                                        },
                                        unexpected)
                                    .commit(
                                        [&]() {
                                            ++writes;
                                        },
                                        unexpected);
                                db.reset();
                            } else if (mode == "error") {
                                sequence = &db->startTransactions(
                                    [&]() {
                                        sequence->exec(
                                            "INSERT INTO items VALUES (1)",
                                            [&]() {
                                                sequence->exec("INVALID SQL", forbidden, [&](const std::string&, unsigned int code) {
                                                    ++errors;
                                                    checks.expectEqual(
                                                        1064, static_cast<int>(code), "original failure reaches entered callback");
                                                    abort();
                                                    // The old sequence is valid until this callback returns, but its new SQL must also be
                                                    // cancelled.
                                                    sequence->exec("INSERT INTO items VALUES (888)", forbidden, unexpected);
                                                    enteredFinished = true;
                                                });
                                            },
                                            unexpected);
                                    },
                                    unexpected);
                                queueWrites();
                            } else if (mode == "waiting") {
                                db->query(
                                    "SELECT SLEEP(20)",
                                    [&](MYSQL_ROW) {
                                        ++lateCallbacks;
                                    },
                                    unexpected);
                                queueWrites();
                                ticks = 2;
                                core::EventReceiver::atNextTick(later);
                            } else {
                                const std::string sql = mode == "stream" ? "SELECT REPEAT('x',4096) FROM seq_1_to_100000" : "SELECT 42";
                                auto& pending = db->query(
                                    sql,
                                    [&](MYSQL_ROW row) {
                                        if (cancelled) {
                                            ++lateCallbacks;
                                        } else if (row != nullptr) {
                                            ++rows;
                                            if (mode == "row" || mode == "stream") {
                                                const std::string value = row[0];
                                                abort();
                                                checks.expectTrue(value == row[0],
                                                                  "row remains alive through entered callback after abort/reset");
                                                enteredFinished = true;
                                            }
                                        } else if (mode == "idle") {
                                            ticks = 3;
                                            core::EventReceiver::atNextTick(later);
                                        } else {
                                            abort();
                                            enteredFinished = true;
                                        }
                                    },
                                    unexpected);
                                if (mode != "idle") {
                                    pending.query(
                                        "SELECT 99",
                                        [&](MYSQL_ROW) {
                                            ++lateCallbacks;
                                        },
                                        unexpected);
                                    queueWrites();
                                }
                            }
                        }
                    }
                },
                "database");
            if (mode == "empty") {
                abort();
            } else {
                db->connect();
                if (mode == "before" || mode == "connecting") {
                    db->exec("INSERT INTO items VALUES (999)", forbidden, unexpected);
                    if (mode == "before") {
                        abort();
                    } else {
                        core::EventReceiver::atNextTick(abort);
                    }
                }
            }
            core::SNodeC::start();
            checks.expectEqual(0, lateCallbacks, "aborted session delivers no further callbacks or queued SQL");
            checks.expectTrue(mode == "drain" || cancelled, "abort occurred in selected case");
            if (mode == "connect" || mode == "error" || mode == "row" || mode == "stream" || mode == "eof") {
                checks.expectTrue(enteredFinished, "entered callback can finish after immediate client destruction");
            }
            if (mode == "reuse") {
                checks.expectEqual(2, connections, "explicit reuse creates a separate new session");
                checks.expectEqual(1, writes, "old cleanup does not affect new session");
            } else if (mode == "drain") {
                checks.expectEqual(3, writes, "ordinary destruction still drains queued transaction");
            }
            if (mode == "row" || mode == "stream" || mode == "eof" || mode == "idle") {
                checks.expectEqual(1, rows, "selected row callback count");
            }
            if (mode == "error" || mode == "connect-error") {
                checks.expectEqual(1, errors, "one original error callback");
            }
            checks.expectEqual(0, mysql_query(reader, "SELECT COALESCE(SUM(id),0) FROM probe.items"), "independent durable-state check");
            MYSQL_RES* records = mysql_store_result(reader);
            MYSQL_ROW row = records != nullptr ? mysql_fetch_row(records) : nullptr;
            const std::string expected = mode == "reuse" ? "7" : mode == "drain" ? "1" : "0";
            checks.expectTrue(row != nullptr && expected == row[0], "no cancelled SQL or unintended transaction subset committed");
            mysql_free_result(records);
            db.reset();
        }
        mysql_close(reader);
        result = checks.processResult();
    }
    return result;
}
