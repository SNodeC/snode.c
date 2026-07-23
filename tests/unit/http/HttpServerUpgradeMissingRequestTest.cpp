#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "log/Logger.h"
#include "tests/support/SemanticLogCapture.h"
#include "tests/support/TestResult.h"
#include "web/http/server/Response.h"
#include "web/http/server/SocketContext.h"

#include <string>

namespace {
    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return "http-upgrade-address";
        }
    };

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        TestSocketConnection()
            : SocketConnection(41, 314, "http-upgrade-server", nullptr) {
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
    tests::support::SemanticLogCapture capture("snodec-http-server-missing-upgrade-request");

    logger::Logger::setLogLevel(6);
    logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
    logger::LogManager::setFormat(logger::LogManager::Format::Json);
    logger::LogManager::freeze();

    TestSocketConnection connection;
    web::http::server::SocketContext context(&connection, {});
    web::http::server::Response response(&context);
    int callbackCount = 0;
    std::string selectedProtocol = "not-empty";

    result.expectTrue(response.isConnected(), "missing upgrade request starts with a connected response");
    result.expectTrue(connection.getSocketContext() == nullptr, "connection starts without an installed upgrade context");

    response.upgrade(nullptr, [&callbackCount, &selectedProtocol](const std::string& protocol) {
        ++callbackCount;
        selectedProtocol = protocol;
    });

    result.expectEqual(1, callbackCount, "missing upgrade request invokes the status callback exactly once");
    result.expectTrue(selectedProtocol.empty(), "missing upgrade request reports no selected protocol");
    result.expectEqual(500, response.statusCode, "missing upgrade request reports HTTP 500");
    result.expectTrue(response.header("Connection") == "close", "missing upgrade request closes the HTTP response connection");
    result.expectTrue(connection.getSocketContext() == nullptr,
                      "missing upgrade request creates no SocketContextUpgrade and performs no context switch");

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
                                   record.value("instance", "") == "http-upgrade-server" &&
                                   record.value("connection", "") == "314";
        }
        if (message.find("SocketContextUpgrade") != std::string::npos || message.find("upgrade plugin") != std::string::npos) {
            ++factoryDiagnosticCount;
        }
    }
    result.expectEqual(1, errorRecordCount, "missing upgrade request emits one error diagnostic");
    result.expectTrue(errorIdentityMatches, "missing upgrade request error uses the bound HTTP server connection identity");
    result.expectEqual(0, factoryDiagnosticCount, "missing upgrade request performs no upgrade factory selection or creation");

    return result.processResult();
}
