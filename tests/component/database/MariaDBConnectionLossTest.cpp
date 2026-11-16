#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "database/mariadb/MariaDBClient.h"
#include "support/TestResult.h"

#include <csignal>
#include <cstdint>
#include <memory>
#include <string>

int main(int argc, char** argv) {
    tests::support::TestResult checks;
    int result = EXIT_FAILURE;
    if (argc == 4) {
        const std::string mode = argv[1];
        core::SNodeC::init(1, argv);
        MYSQL* killer = mysql_init(nullptr);
        if (mysql_real_connect(killer, "localhost", argv[3], "", nullptr, 0, argv[2], 0) != nullptr) {
            checks.expectEqual(0, mysql_query(killer, "CREATE DATABASE probe"), "create isolated database");
            checks.expectEqual(0, mysql_query(killer, "CREATE TABLE probe.items(id INT)"), "create cancellation probe");
            database::mariadb::MariaDBConnectionDetails details;
            details.hostname = "localhost";
            details.username = argv[3];
            details.socket = argv[2];
            details.database = "probe";
            details.connectionName = "loss-test";
            std::unique_ptr<database::mariadb::MariaDBClient> db;
            std::unique_ptr<core::timer::Timer> killTimer, deadline;
            unsigned connections = 0, losses = 0, errors = 0, rows = 0, interrupted = 0, late = 0;
            std::uint64_t victim = 0;
            const auto finish = [&] {
                db->abort();
                core::SNodeC::stop();
            };
            const auto unexpected = [&](const std::string&, unsigned) {
                ++errors;
                finish();
            };
            const auto recover = [&] {
                db->query(
                    "SELECT 42",
                    [&](MYSQL_ROW row) {
                        if (row != nullptr && row[0] != nullptr && std::string(row[0]) == "42") {
                            ++rows;
                        } else if (row == nullptr) {
                            finish();
                        }
                    },
                    unexpected);
            };
            db = std::make_unique<database::mariadb::MariaDBClient>(details, [&](const auto& state) {
                if (state.connected) {
                    ++connections;
                } else {
                    ++losses;
                    if (mode == "idle") {
                        recover();
                    }
                }
            });
            db->query(
                "SELECT CONNECTION_ID()",
                [&](MYSQL_ROW row) {
                    if (row != nullptr) {
                        victim = std::stoull(row[0]);
                    } else if (mode == "cancel") {
                        finish();
                    } else {
                        if (mode == "query" || mode == "shutdown-query") {
                            db->query(
                                "SELECT SLEEP(30)",
                                [&](MYSQL_ROW) {
                                    ++errors;
                                    finish();
                                },
                                [&](const std::string&, unsigned error) {
                                    checks.expectEqual(2013, static_cast<int>(error), "entered query reports connection loss");
                                    ++interrupted;
                                    recover();
                                });
                            db->exec(
                                "INSERT INTO items VALUES (999)",
                                [&] {
                                    ++late;
                                },
                                [&](const std::string&, unsigned) {
                                    ++late;
                                });
                        }
                        killTimer = std::make_unique<core::timer::Timer>(core::timer::Timer::singleshotTimer(
                            [&] {
                                if (mode == "shutdown" || mode == "shutdown-query") {
                                    std::raise(SIGTERM);
                                } else {
                                    const auto sql = "KILL CONNECTION " + std::to_string(victim);
                                    checks.expectEqual(0, mysql_query(killer, sql.c_str()), "kill only the disposable test connection");
                                }
                            },
                            .1));
                    }
                },
                unexpected);
            deadline = std::make_unique<core::timer::Timer>(core::timer::Timer::singleshotTimer(finish, 3));
            checks.expectEqual(mode == "shutdown" || mode == "shutdown-query" ? -SIGTERM : 0,
                               core::SNodeC::start(),
                               "framework preserves the signal result for its application");
            const bool recovery = mode == "idle" || mode == "query";
            checks.expectEqual(recovery ? 2 : 1, static_cast<int>(connections), "recovery creates a new session");
            checks.expectEqual(recovery ? 1 : 0, static_cast<int>(rows), "recovery query returns its value");
            checks.expectEqual(mode == "idle" ? 1 : 0, static_cast<int>(losses), "one appropriate state-loss notification");
            checks.expectEqual(mode == "query" ? 1 : 0, static_cast<int>(interrupted), "one active error callback");
            checks.expectEqual(0, static_cast<int>(errors + late), "failed session delivers no stale callbacks");
            checks.expectEqual(0, mysql_query(killer, "SELECT COUNT(*) FROM probe.items"), "read independent durable state");
            MYSQL_RES* records = mysql_store_result(killer);
            MYSQL_ROW row = records != nullptr ? mysql_fetch_row(records) : nullptr;
            checks.expectTrue(row != nullptr && std::string(row[0]) == "0", "unsent SQL was not replayed on recovery");
            mysql_free_result(records);
            db.reset();
        } else {
            checks.expectTrue(false, "independent fixture connection opens");
        }
        mysql_close(killer);
        core::SNodeC::free();
        result = checks.processResult();
    }
    return result;
}
