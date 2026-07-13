#include "log/Logger.h"
#include "log/SemanticLogger.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <chrono>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace {
    void expectStringEqual(tests::support::TestResult& result,
                           const std::string& expected,
                           const std::string& actual,
                           const std::string& message) {
        result.expectTrue(expected == actual, message + " expected='" + expected + "' actual='" + actual + "'");
    }

    std::chrono::system_clock::time_point fixedTimestamp() {
        using namespace std::chrono;
        return system_clock::time_point{seconds{1783254896} + milliseconds{789}};
    }
} // namespace

int main() {
    tests::support::TestResult result;

    std::string component = "mqtt.server";
    std::string instance = "mqtt-broker";
    std::string connection = "#7";
    logger::LogScope borrowed{
        logger::LogOrigin::Framework, logger::LogBoundary::Connection, component, instance, logger::LogRole::Server, connection};
    auto owned = logger::materialize(borrowed, logger::LogLevel::Info, "ready", logger::LogRecordOptions{.ts = fixedTimestamp()});
    component.clear();
    instance.clear();
    connection.clear();
    expectStringEqual(result, "mqtt.server", owned.component, "materialize copies component string_view");
    result.expectTrue(owned.instance && *owned.instance == "mqtt-broker", "materialize copies instance string_view");
    result.expectTrue(owned.connection && *owned.connection == "#7", "materialize copies connection string_view");
    result.expectTrue(owned.role && *owned.role == logger::LogRole::Server, "known role is represented as optional role");

    logger::LogScope unknownRole{
        logger::LogOrigin::Application, logger::LogBoundary::Context, "EchoServerContext", "", logger::LogRole::Unknown, ""};
    auto minimal =
        logger::materialize(unknownRole, logger::LogLevel::Info, "echo session started", logger::LogRecordOptions{.ts = fixedTimestamp()});
    const std::string json = logger::formatJsonV1(minimal);
    result.expectTrue(json.find("\"v\":1") != std::string::npos, "JSON emits mandatory v");
    result.expectTrue(json.find("\"ts\":\"2026-07-05T12:34:56.789Z\"") != std::string::npos, "JSON emits mandatory ts timestamp");
    result.expectTrue(json.find("\"level\":\"info\"") != std::string::npos, "JSON emits lowercase level");
    result.expectTrue(json.find("\"origin\":\"application\"") != std::string::npos, "JSON emits lowercase origin");
    result.expectTrue(json.find("\"boundary\":\"context\"") != std::string::npos, "JSON emits lowercase boundary");
    result.expectTrue(json.find("\"component\":\"EchoServerContext\"") != std::string::npos, "JSON emits mandatory component");
    result.expectTrue(json.find("\"message\":\"echo session started\"") != std::string::npos, "JSON emits mandatory message");
    result.expectTrue(json.find("instance") == std::string::npos, "JSON omits absent instance instead of null");
    result.expectTrue(json.find("role") == std::string::npos, "JSON omits unknown role");
    result.expectTrue(json.find("connection") == std::string::npos, "JSON omits absent connection instead of null");
    result.expectTrue(json.find("null") == std::string::npos, "JSON never emits null for absent optionals");

    auto escapedRecord = logger::materialize(unknownRole,
                                             logger::LogLevel::Info,
                                             std::string("before") + static_cast<char>(0x08) + static_cast<char>(0x01) + "after",
                                             logger::LogRecordOptions{.ts = fixedTimestamp()});
    const std::string escapedJson = logger::formatJsonV1(escapedRecord);
    result.expectTrue(escapedJson.find("before\\b\\u0001after") != std::string::npos,
                      "JSON escapes backspace and other control characters");
    result.expectTrue(escapedJson.find(static_cast<char>(0x08)) == std::string::npos &&
                          escapedJson.find(static_cast<char>(0x01)) == std::string::npos,
                      "JSON does not emit raw control bytes below 0x20");

    const std::string text = logger::formatText(owned);
    expectStringEqual(result,
                      "2026-07-05T12:34:56.789Z INF framework/connection mqtt.server role=server inst=mqtt-broker conn=#7 — ready",
                      text,
                      "text formatter uses final semantic grammar");
    result.expectTrue(logger::formatText(owned) == logger::formatText(owned, false), "plain overloads are byte-equivalent");
    const std::string coloredText = logger::formatText(owned, true);
    result.expectTrue(coloredText.find("\033[") != std::string::npos, "colored formatter emits ANSI SGR");

    auto emptyMessage = logger::materialize(unknownRole, logger::LogLevel::Info, "", logger::LogRecordOptions{.ts = fixedTimestamp()});
    expectStringEqual(result,
                      "2026-07-05T12:34:56.789Z INF application/context EchoServerContext",
                      logger::formatText(emptyMessage),
                      "empty semantic messages omit separator and trailing whitespace");

    logger::LogRecord rich = owned;
    rich.event = std::string("http.response");
    rich.error = logger::LogError{5, "Input/output error"};
    rich.source = logger::LogSource{"SocketContext.cpp", 123, "onRead"};
    rich.connection = std::string("[7] mqtt client");
    expectStringEqual(
        result,
        R"(2026-07-05T12:34:56.789Z INF framework/connection mqtt.server role=server inst=mqtt-broker conn="[7] mqtt client" event=http.response error="5:Input/output error" src=SocketContext.cpp:123:onRead — ready)",
        logger::formatText(rich),
        "rich text formatter renders deterministic fields before the message");

    logger::LogRecord escaped = emptyMessage;
    escaped.instance = std::string("space = | , ; [ ] \" \\");
    escaped.connection = std::string("line\ncr\rtab\tbs\bff\f") + static_cast<char>(0) + static_cast<char>(0x1B) + static_cast<char>(0x7F);
    escaped.event = std::string("");
    escaped.message = std::string("first\r\n\nthird\nraw") + static_cast<char>(0x1B) + " utf8=µ";
    const std::string escapedText = logger::formatText(escaped);
    result.expectTrue(escapedText.find("inst=\"") != std::string::npos && escapedText.find("\\\"") != std::string::npos &&
                          escapedText.find("\\\\") != std::string::npos,
                      "quoted fields escape quotes and backslashes");
    result.expectTrue(escapedText.find("event=\"\"") != std::string::npos, "present-empty optionals render quoted empty strings");
    result.expectTrue(escapedText.find("\\x00") != std::string::npos && escapedText.find("\\x1B") != std::string::npos &&
                          escapedText.find("\\x7F") != std::string::npos,
                      "control bytes are escaped as deterministic hex");
    result.expectTrue(escapedText.find("— first\n│ \n│ third\n│ raw\\x1B utf8=µ") != std::string::npos,
                      "CRLF, blank lines, trailing markers, ESC sanitization, and UTF-8 messages are preserved");
    result.expectTrue(escapedText.find(static_cast<char>(0x1B)) == std::string::npos, "raw ESC never reaches plain semantic output");

    std::vector<logger::LogRecord> records;
    logger::LogScope scope{logger::LogOrigin::Framework,
                           logger::LogBoundary::Connection,
                           "core.socket.stream",
                           "test-instance",
                           logger::LogRole::Server,
                           "tcp://127.0.0.1:8080"};
    auto log = logger::BoundaryLogger::createForTest(
        scope,
        [&](logger::LogRecord record) {
            records.push_back(std::move(record));
        },
        logger::LogLevel::Debug,
        fixedTimestamp);
    log.debug("received {} bytes", 42);
    log.info("echo session started");
    log.debug() << "received " << 42 << " bytes";
    result.expectEqual(3, static_cast<int>(records.size()), "format and stream APIs emit expected number of complete records");
    expectStringEqual(result, "received 42 bytes", records[0].message, "format-style API formats placeholder");
    expectStringEqual(result, "echo session started", records[1].message, "format-style API supports literal message");
    expectStringEqual(result, "received 42 bytes", records[2].message, "stream-style API flushes complete expression once");

    std::string loggerComponent = "mqtt.server";
    std::string loggerInstance = "mqtt-broker";
    std::string loggerConnection = "#7";
    std::vector<logger::LogRecord> lifetimeRecords;
    auto lifetimeLog = logger::BoundaryLogger::createForTest(
        logger::LogScope{logger::LogOrigin::Framework,
                         logger::LogBoundary::Connection,
                         loggerComponent,
                         loggerInstance,
                         logger::LogRole::Server,
                         loggerConnection},
        [&](logger::LogRecord record) {
            lifetimeRecords.push_back(std::move(record));
        },
        logger::LogLevel::Trace,
        fixedTimestamp);
    loggerComponent = "changed.component";
    loggerInstance = "changed-instance";
    loggerConnection = "changed-connection";
    lifetimeLog.info("after mutation");
    result.expectEqual(1, static_cast<int>(lifetimeRecords.size()), "BoundaryLogger emits after original scope strings mutate");
    expectStringEqual(result, "mqtt.server", lifetimeRecords[0].component, "BoundaryLogger copies component at construction");
    result.expectTrue(lifetimeRecords[0].instance && *lifetimeRecords[0].instance == "mqtt-broker",
                      "BoundaryLogger copies instance at construction");
    result.expectTrue(lifetimeRecords[0].connection && *lifetimeRecords[0].connection == "#7",
                      "BoundaryLogger copies connection at construction");

    auto filtered = logger::BoundaryLogger::createForTest(
        scope,
        [&](logger::LogRecord record) {
            records.push_back(std::move(record));
        },
        logger::LogLevel::Info,
        fixedTimestamp);
    filtered.debug("hidden {}", 1);
    filtered.debug() << "hidden " << 2;
    result.expectEqual(3, static_cast<int>(records.size()), "local threshold suppresses disabled format and stream calls");

    log.sysError(logger::LogLevel::Error, 98, "bind failed on {}", "127.0.0.1:8080");
    result.expectTrue(records.back().error && records.back().error->code == 98, "sysError(int) stores error code");
    result.expectTrue(records.back().error && !records.back().error->text.empty(), "sysError(int) stores error text");
    expectStringEqual(
        result, "bind failed on 127.0.0.1:8080", records.back().message, "sysError formats message separately from error object");
    const std::string errorJson = logger::formatJsonV1(records.back());
    result.expectTrue(errorJson.find("\"error\":{") != std::string::npos && errorJson.find("\"code\":98") != std::string::npos,
                      "JSON emits error only when present");

    log.sysError(logger::LogLevel::Warn, std::make_error_code(std::errc::permission_denied), "open failed");
    result.expectTrue(records.back().error && records.back().error->code != 0, "sysError(std::error_code) stores error code");

    logger::Logger::init();
    logger::Logger::setDisableColor(true);
    logger::Logger::setTickResolver([] {
        return "000000";
    });
    logger::Logger::setLogLevel(1);
    logger::Logger::setVerboseLevel(1);
    LOG(INFO) << "round2 compatibility LOG enabled";
    errno = EACCES;
    PLOG(ERROR) << "round2 compatibility PLOG enabled";
    LOG(TRACE) << "round2 compatibility LOG disabled";
    result.expectTrue(true, "legacy LOG/PLOG macros compile/link/run via existing macro path");

    return result.processResult();
}
