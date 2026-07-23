#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "log/Logger.h"
#include "tests/support/Phase3SemanticLogCapture.h"
#include "tests/support/TestResult.h"
#include "web/http/server/Response.h"
#include "web/http/server/SemanticLog.h"
#include "web/http/server/SocketContext.h"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace {
    struct ExpensiveValue {
        int* evaluations;
    };

    std::ostream& operator<<(std::ostream& out, const ExpensiveValue& value) {
        ++*value.evaluations;
        return out << "expensive";
    }

    class LoggerStateGuard {
    public:
        explicit LoggerStateGuard(const std::string& logFile) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setVerboseLevel(0);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::setTickResolver([]() {
                return std::string("MIGRATION07BTICK");
            });
            logger::Logger::logToFile(logFile);
        }

        ~LoggerStateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::setTickResolver({});
            logger::Logger::init();
            logger::LogManager::init();
            errno = 0;
        }
    };

    std::filesystem::path tempLogPath(const std::string& name) {
        auto path = std::filesystem::temp_directory_path() / name;
        std::error_code error;
        std::filesystem::remove(path, error);
        std::filesystem::remove(path.string() + ".1", error);
        return path;
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream in(path);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return "phase3-http-upgrade-address";
        }
    };

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        TestSocketConnection()
            : SocketConnection(41, 314, "phase3-http-upgrade-server", nullptr) {
        }

        int getFd() const override {
            return 41;
        }
        void sendToPeer(const char*, std::size_t) override {
        }
        bool streamToPeer(core::pipe::Source*) override {
            return false;
        }
        void streamEof() override {
        }
        std::size_t readFromPeer(char*, std::size_t) override {
            return 0;
        }
        void shutdownRead() override {
        }
        void shutdownWrite() override {
        }
        const core::socket::SocketAddress& getBindAddress() const override {
            return testAddress();
        }
        const core::socket::SocketAddress& getLocalAddress() const override {
            return testAddress();
        }
        const core::socket::SocketAddress& getRemoteAddress() const override {
            return testAddress();
        }
        void close() override {
        }
        void setTimeout(const utils::Timeval&) override {
        }
        void setReadTimeout(const utils::Timeval&) override {
        }
        void setWriteTimeout(const utils::Timeval&) override {
        }
        std::size_t getTotalSent() const override {
            return 0;
        }
        std::size_t getTotalQueued() const override {
            return 0;
        }
        std::size_t getTotalRead() const override {
            return 0;
        }
        std::size_t getTotalProcessed() const override {
            return 0;
        }

    private:
        static const core::socket::SocketAddress& testAddress() {
            static const TestSocketAddress address;
            return address;
        }
    };
} // namespace

int main() {
    tests::support::TestResult result;

    const auto enabledPath = tempLogPath("snodec-migration07b-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        web::http::server::semantic::httpServerLog().debug("http server semantic logger emitted");
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("http server semantic logger emitted") != std::string::npos,
                      "HTTP server semantic logger emits when enabled");
    result.expectTrue(enabledLog.find(" framework/connection web.http.server ") != std::string::npos,
                      "records carry framework web.http.server component scope");

    const auto upgradeIdentityPath = tempLogPath("snodec-phase3-http-server-upgrade-identity.log");
    {
        LoggerStateGuard guard(upgradeIdentityPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::freeze();
        TestSocketConnection connection;
        web::http::server::semantic::httpServerLog(connection).debug("Initiating upgrade: GET /phase3/upgrade HTTP/1.1 protocol=websocket");
    }
    const std::string upgradeIdentityLog = readFile(upgradeIdentityPath);
    const std::string textSeparator = " — ";
    const std::size_t separator = upgradeIdentityLog.find(textSeparator);
    const std::string upgradePayload =
        separator == std::string::npos ? std::string() : upgradeIdentityLog.substr(separator + textSeparator.size());
    result.expectTrue(upgradeIdentityLog.find(" framework/connection web.http.server ") != std::string::npos &&
                          upgradeIdentityLog.find("role=server") != std::string::npos &&
                          upgradeIdentityLog.find("inst=phase3-http-upgrade-server") != std::string::npos &&
                          upgradeIdentityLog.find("conn=314") != std::string::npos,
                      "bound HTTP upgrade text carries structured component, role, instance, and connection identity");
    result.expectTrue(upgradePayload.starts_with("Initiating upgrade:") &&
                          upgradePayload.find("GET /phase3/upgrade HTTP/1.1 protocol=websocket") != std::string::npos &&
                          upgradePayload.find("phase3-http-upgrade-server") == std::string::npos &&
                          upgradePayload.find("HTTP:") == std::string::npos && upgradePayload.find("HTTP upgrade:") == std::string::npos,
                      "text payload starts with upgrade content and does not repeat architectural identity");

    {
        tests::support::Phase3SemanticLogCapture capture("snodec-phase3-http-server-null-upgrade-request");
        logger::Logger::setLogLevel(6);
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();

        TestSocketConnection connection;
        web::http::server::SocketContext context(&connection, {});
        web::http::server::Response response(&context);
        int callbackCount = 0;
        std::string selectedProtocol = "not-empty";

        result.expectTrue(response.isConnected(), "null upgrade request regression starts with a connected response");
        result.expectTrue(connection.getSocketContext() == nullptr, "fake connection starts without an installed upgrade context");

        response.upgrade(nullptr, [&callbackCount, &selectedProtocol](const std::string& protocol) {
            ++callbackCount;
            selectedProtocol = protocol;
        });

        result.expectEqual(1, callbackCount, "null upgrade request invokes the status callback exactly once");
        result.expectTrue(selectedProtocol.empty(), "null upgrade request reports no selected protocol");
        result.expectEqual(500, response.statusCode, "null upgrade request reports HTTP 500");
        result.expectTrue(response.header("Connection") == "close", "null upgrade request closes the HTTP response connection");
        result.expectTrue(connection.getSocketContext() == nullptr,
                          "null upgrade request does not create or switch an upgrade context");

        const auto records = capture.finish();
        int errorRecordCount = 0;
        int factoryDiagnosticCount = 0;
        bool errorIdentityMatches = false;
        for (const auto& record : records) {
            const std::string message = record.value("message", "");
            if (message == "Upgrade request has gone away") {
                ++errorRecordCount;
                errorIdentityMatches = record.value("level", "") == "error" && record.value("origin", "") == "framework" &&
                                       record.value("boundary", "") == "connection" &&
                                       record.value("component", "") == "web.http.server" && record.value("role", "") == "server" &&
                                       record.value("instance", "") == "phase3-http-upgrade-server" &&
                                       record.value("connection", "") == "314";
            }
            if (message.find("SocketContextUpgrade") != std::string::npos || message.find("upgrade plugin") != std::string::npos) {
                ++factoryDiagnosticCount;
            }
        }
        result.expectEqual(1, errorRecordCount, "null upgrade request emits one error diagnostic");
        result.expectTrue(errorIdentityMatches, "null upgrade request error uses the bound HTTP server identity");
        result.expectEqual(0, factoryDiagnosticCount, "null upgrade request does not invoke upgrade factory selection or creation");
    }

    const auto managerFilterPath = tempLogPath("snodec-migration07b-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        web::http::server::semantic::httpServerLog().debug("http server suppressed by manager");
    }
    result.expectTrue(readFile(managerFilterPath).find("suppressed by manager") == std::string::npos,
                      "LogManager filtering suppresses migrated HTTP server semantic calls");

    const auto backendFilterPath = tempLogPath("snodec-migration07b-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        web::http::server::semantic::httpServerLog().debug("http server suppressed by backend");
    }
    result.expectTrue(readFile(backendFilterPath).find("suppressed by backend") != std::string::npos,
                      "semantic output accepted by LogManager is not suppressed by Logger::setLogLevel");

    const auto jsonPath = tempLogPath("snodec-migration07b-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        web::http::server::semantic::httpServerUpgradeLog().debug("http server upgrade json format respected");
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"http server upgrade json format respected\"") != std::string::npos &&
                          jsonLog.find("\"origin\":\"framework\"") != std::string::npos &&
                          jsonLog.find("\"component\":\"web.http.server.upgrade\"") != std::string::npos,
                      "JSON format selection is respected");

    std::vector<logger::LogRecord> sysErrorRecords;
    web::http::server::semantic::httpServerLog([&](logger::LogRecord record) {
        sysErrorRecords.push_back(std::move(record));
    }).sysError(logger::LogLevel::Error, EPIPE, "{} HTTP: response write failed", "server-conn-07b");
    result.expectTrue(sysErrorRecords.size() == 1 && sysErrorRecords[0].error && sysErrorRecords[0].error->code == EPIPE &&
                          sysErrorRecords[0].error->text.find("Broken pipe") != std::string::npos,
                      "sysError errno code/text is preserved by HTTP server helper");

    std::vector<logger::LogRecord> sinkRecords;
    web::http::server::semantic::httpServerUpgradeLog([&](logger::LogRecord record) {
        sinkRecords.push_back(std::move(record));
    }).warn("HTTP upgrade: Overriding http upgrade library dir");
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].component == "web.http.server.upgrade" &&
                          sinkRecords[0].message == "HTTP upgrade: Overriding http upgrade library dir",
                      "sink-taking overload remains compatible");

    const auto suppressedPath = tempLogPath("snodec-migration07b-suppressed.log");
    {
        LoggerStateGuard guard(suppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        int evaluations = 0;
        web::http::server::semantic::httpServerLog().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production formatting does not evaluate ExpensiveValue after PR #151");
    }

    std::vector<logger::LogRecord> requestRecords;
    web::http::server::semantic::httpServerLog([&](logger::LogRecord record) {
        requestRecords.push_back(std::move(record));
    }).info("{} HTTP: Request parse success: {} {} HTTP/{}.{}", "server-conn-07b", "GET", "/resource", 1, 1);
    result.expectTrue(requestRecords.size() == 1 &&
                          requestRecords[0].message.find("server-conn-07b HTTP: Request parse success: GET /resource HTTP/1.1") !=
                              std::string::npos,
                      "representative HTTP request parse-success payload preservation is covered");

    std::vector<logger::LogRecord> responseRecords;
    web::http::server::semantic::httpServerLog([&](logger::LogRecord record) {
        responseRecords.push_back(std::move(record));
    })
        .info("{} HTTP: Response completed for request: {} {} HTTP/{}.{}; HTTP/{}.{} {} {}",
              "server-conn-07b",
              "GET",
              "/resource",
              1,
              1,
              1,
              1,
              200,
              "OK");
    result.expectTrue(responseRecords.size() == 1 && responseRecords[0].message.find("HTTP/1.1 200 OK") != std::string::npos,
                      "representative HTTP response payload preservation is covered");

    std::vector<logger::LogRecord> upgradeRecords;
    web::http::server::semantic::httpServerUpgradeLog([&](logger::LogRecord record) {
        upgradeRecords.push_back(std::move(record));
    })
        .debug("{} HTTP: Upgrade bootstrap {}; Protocol selected: {}; requested: {}; Subprotocol selected: {}; requested: {}",
               "server-conn-07b",
               "success",
               "websocket",
               "websocket",
               "chat",
               "chat");
    result.expectTrue(upgradeRecords.size() == 1 && upgradeRecords[0].component == "web.http.server.upgrade" &&
                          upgradeRecords[0].message.find("Protocol selected: websocket") != std::string::npos,
                      "representative HTTP server-upgrade payload preservation is covered");

    std::vector<logger::LogRecord> errorRecords;
    web::http::server::semantic::httpServerLog([&](logger::LogRecord record) {
        errorRecords.push_back(std::move(record));
    })
        .error("{} HTTP: Request parse error: {} ({}) / HTTP upgrade: Request has gone away / HTTP upgrade: Unexpected disconnect",
               "server-conn-07b",
               "Bad Request",
               400);
    result.expectTrue(errorRecords.size() == 1 &&
                          errorRecords[0].message.find("Request parse error: Bad Request (400)") != std::string::npos &&
                          errorRecords[0].message.find("Request has gone away") != std::string::npos &&
                          errorRecords[0].message.find("Unexpected disconnect") != std::string::npos,
                      "representative parse-error/gone-away/unexpected-disconnect payload preservation is covered");

    return result.processResult();
}
