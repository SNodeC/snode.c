#include "log/LogScopeOwner.h"
#include "tests/support/TestResult.h"

#include <chrono>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
    void expectStringEqual(tests::support::TestResult& result,
                           const std::string& expected,
                           std::string_view actual,
                           const std::string& message) {
        result.expectTrue(expected == actual, message + " expected='" + expected + "' actual='" + std::string(actual) + "'");
    }

    std::chrono::system_clock::time_point fixedTimestamp() {
        using namespace std::chrono;
        return system_clock::time_point{seconds{1783254896} + milliseconds{789}};
    }

    logger::LogScopeOwner makeOwnerFromDestroyedStrings() {
        std::string component = "core.socket.stream";
        std::string instance = "echo-server";
        std::string connection = "#7";
        logger::LogScope borrowed{
            logger::LogOrigin::Framework, logger::LogBoundary::Connection, component, instance, logger::LogRole::Server, connection};
        return logger::LogScopeOwner::fromScope(borrowed);
    }
} // namespace

int main() {
    tests::support::TestResult result;

    static_assert(std::is_copy_constructible_v<logger::LogScopeOwner>, "LogScopeOwner copy behavior is intentionally supported");
    static_assert(std::is_copy_assignable_v<logger::LogScopeOwner>, "LogScopeOwner copy assignment is intentionally supported");
    static_assert(std::is_move_constructible_v<logger::LogScopeOwner>, "LogScopeOwner move behavior is intentionally supported");
    static_assert(std::is_move_assignable_v<logger::LogScopeOwner>, "LogScopeOwner move assignment is intentionally supported");

    std::string component = "core.socket.stream";
    std::string instance = "echo-server";
    std::string connection = "#7";
    logger::LogScope borrowed{
        logger::LogOrigin::Framework, logger::LogBoundary::Connection, component, instance, logger::LogRole::Server, connection};

    auto owner = logger::LogScopeOwner::fromScope(borrowed);

    component = "changed.component";
    instance = "changed-instance";
    connection = "changed-connection";

    auto scope = owner.scope();
    result.expectTrue(scope.origin == logger::LogOrigin::Framework, "owner.scope exposes original origin");
    result.expectTrue(scope.boundary == logger::LogBoundary::Connection, "owner.scope exposes original boundary");
    expectStringEqual(result, "core.socket.stream", scope.component, "owner copies borrowed component string");
    expectStringEqual(result, "echo-server", scope.instance, "owner copies borrowed instance string");
    result.expectTrue(scope.role == logger::LogRole::Server, "owner.scope exposes known role");
    expectStringEqual(result, "#7", scope.connection, "owner copies borrowed connection string");

    auto destroyedOwner = makeOwnerFromDestroyedStrings();
    auto destroyedScope = destroyedOwner.scope();
    expectStringEqual(result, "core.socket.stream", destroyedScope.component, "owner remains valid after source component is destroyed");
    expectStringEqual(result, "echo-server", destroyedScope.instance, "owner remains valid after source instance is destroyed");
    expectStringEqual(result, "#7", destroyedScope.connection, "owner remains valid after source connection is destroyed");

    std::vector<logger::LogRecord> records;
    auto log = owner.logger(
        [&](logger::LogRecord record) {
            records.push_back(std::move(record));
        },
        logger::LogLevel::Trace,
        fixedTimestamp);
    log.info("hello");
    result.expectEqual(1, static_cast<int>(records.size()), "BoundaryLogger created from owner emits one record");
    expectStringEqual(result, "core.socket.stream", records[0].component, "logger from owner emits original component");
    result.expectTrue(records[0].instance && *records[0].instance == "echo-server", "logger from owner emits original instance");
    result.expectTrue(records[0].role && *records[0].role == logger::LogRole::Server, "logger from owner emits original known role");
    result.expectTrue(records[0].connection && *records[0].connection == "#7", "logger from owner emits original connection");

    logger::LogScope unknownRole{
        logger::LogOrigin::Application, logger::LogBoundary::Context, "EchoServerContext", "", logger::LogRole::Unknown, ""};
    auto unknownOwner = logger::LogScopeOwner::fromScope(unknownRole);
    auto unknownScope = unknownOwner.scope();
    result.expectTrue(unknownScope.role == logger::LogRole::Unknown,
                      "unknown role remains absent in owner and appears as Unknown in borrowed view");
    auto unknownRecord =
        logger::materialize(unknownScope, logger::LogLevel::Info, "hello", logger::LogRecordOptions{.ts = fixedTimestamp()});
    const std::string unknownJson = logger::formatJsonV1(unknownRecord);
    result.expectTrue(unknownJson.find("role") == std::string::npos, "unknown role remains omitted from JSON");
    result.expectTrue(unknownJson.find("unknown") == std::string::npos, "unknown role string is not emitted in JSON");

    auto copiedOwner = owner;
    component = "copy-source-changed";
    auto copiedScope = copiedOwner.scope();
    expectStringEqual(result, "core.socket.stream", copiedScope.component, "copied owner has independent component storage");
    expectStringEqual(result, "echo-server", copiedScope.instance, "copied owner has independent instance storage");
    expectStringEqual(result, "#7", copiedScope.connection, "copied owner has independent connection storage");

    auto movedOwner = std::move(copiedOwner);
    auto movedScope = movedOwner.scope();
    expectStringEqual(result, "core.socket.stream", movedScope.component, "moved-to owner remains valid for component");
    expectStringEqual(result, "echo-server", movedScope.instance, "moved-to owner remains valid for instance");
    expectStringEqual(result, "#7", movedScope.connection, "moved-to owner remains valid for connection");

    return result.processResult();
}
