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
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    int result = tests::support::cTestSkipReturnCode;
    const char* selected = argc > 1 ? argv[1] : nullptr;
    const char* socket = argc > 2 ? argv[2] : nullptr;
    const char* user = argc > 3 ? argv[3] : nullptr;
    const std::string mode = selected != nullptr ? selected : "idle";
    const bool failure = mode == "failure" || mode == "lazy-failure";
    const bool fixture = mode != "idle" && !failure;

    if (argc != 4) {
        std::cerr << "Run this test through CTest to create its disposable MariaDB fixture" << std::endl;
        result = EXIT_FAILURE;
    } else if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("MariaDBConnectTest");
    } else {
        core::SNodeC::init(1, argv);
        int connections = 0, connectionErrors = 0, queryErrors = 0, rows = 0, executions = 0, fieldReads = 0;
        bool started = false;
        std::unique_ptr<database::mariadb::MariaDBClient> db;
        const auto queryError = [&](const std::string&, unsigned int error) {
            testResult.expectTrue(error != 0, "query failure supplies a MariaDB error");
            ++queryErrors;
            db.reset();
        };
        const auto query = [&]() {
            db->query(
                mode == "lazy-exec" ? "SELECT @snodec_connect_test" : "SELECT 42",
                [&](const MYSQL_ROW row) {
                    if (row != nullptr) {
                        testResult.expectTrue(row[0] != nullptr && std::string(row[0]) == "42", "first query returns its expected value");
                        ++rows;
                        db->fieldCount(
                            [&](unsigned int count) {
                                testResult.expectEqual(1, static_cast<int>(count), "synchronous metadata uses the established connection");
                                ++fieldReads;
                            },
                            queryError);
                    } else {
                        db.reset();
                    }
                },
                queryError);
        };
        const database::mariadb::MariaDBConnectionDetails details{
            .connectionName = "explicit-connect-test",
            .hostname = "localhost",
            .username = user != nullptr ? user : "unused",
            .password = "",
            .database = "",
            .port = 0,
            .socket = failure ? std::string(socket != nullptr ? socket : "/tmp/snodec-mariadb") + ".missing-" + std::to_string(getpid())
                              : std::string(socket != nullptr ? socket : ""),
            .flags = 0};
        db = std::make_unique<database::mariadb::MariaDBClient>(details, [&](const database::mariadb::MariaDBState& state) {
            testResult.expectTrue(started, "connection notification is deferred until the event loop runs");
            if (state.connected) {
                ++connections;
                if (mode == "explicit" || mode == "repeated") {
                    if (mode == "repeated") {
                        db->connect();
                        db->connect();
                    }
                    query();
                }
            } else if (state.error != 0) {
                ++connectionErrors;
            }
        });
        if (mode == "explicit" || mode == "repeated" || mode == "queued" || mode == "failure") {
            db->connect();
            if (mode == "repeated" || mode == "queued") {
                db->connect();
                db->connect();
            }
        }
        if (mode == "queued" || mode == "lazy-query" || mode == "lazy-failure") {
            query();
        } else if (mode == "lazy-exec") {
            db->exec(
                "SET @snodec_connect_test = 42",
                [&]() {
                    ++executions;
                    query();
                },
                queryError);
        }
        testResult.expectEqual(0, connections + connectionErrors + rows + executions, "registration does not run callbacks synchronously");
        started = true;
        testResult.expectEqual(0, core::SNodeC::start(), "event loop returns naturally without an explicit stop");
        testResult.expectEqual(fixture ? 1 : 0, connections, "one connection for every successful explicit or lazy case");
        testResult.expectEqual(failure ? 1 : 0, connectionErrors, "connection failure is reported once");
        // The existing API reports initial connection failure through onStateChanged, without executing queued queries.
        testResult.expectEqual(0, queryErrors, "connection failure preserves the existing notification route");
        testResult.expectEqual(fixture ? 1 : 0, rows, "successful case executes the first query exactly once");
        testResult.expectEqual(fixture ? 1 : 0, fieldReads, "synchronous metadata is read exactly once");
        testResult.expectEqual(mode == "lazy-exec" ? 1 : 0, executions, "lazy exec executes exactly once");
        db.reset();
        core::SNodeC::free();
        result = testResult.processResult();
    }

    return result;
}
